
#include "Etterna/Globals/global.h"
#include "Profile.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "DBProfile.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/SongManager.h"
#include "sqlite3.h"
#include <SQLiteCpp/SQLiteCpp.h>

#include <algorithm>

const string PROFILE_DB = "profile.db";
const string WRITE_ONLY_PROFILE_DB = "webprofile.db";

ProfileLoadResult
DBProfile::LoadDBFromDir(const std::string& dir, Profile* profile)
{
	loadingProfile = profile;
	return LoadDBFromDir(dir);
}

ProfileLoadResult
DBProfile::LoadDBFromDir(const std::string& dir)
{
	SQLite::Database* db = nullptr;
	try {
		// Open a database file
		db = new SQLite::Database(FILEMAN->ResolvePath(dir) + PROFILE_DB,
								  SQLite::OPEN_READWRITE);
	} catch (std::exception&) {
		return ProfileLoadResult_FailedNoProfile;
	}
	try {
		// Use the db
		// if(!LoadGeneralData(db))
		//	return ProfileLoadResult_FailedTampered;
		LoadFavourites(db);
		LoadPermaMirrors(db);
		LoadScoreGoals(db);
		LoadPlayLists(db);
		LoadPlayerScores(db);
	} catch (std::exception&) {
		return ProfileLoadResult_FailedTampered;
	}
	delete db;
	return ProfileLoadResult_Success;
}

bool
DBProfile::LoadGeneralData(SQLite::Database* db)
{
	SQLite::Statement gDataQuery(*db, "SELECT * FROM generaldata");

	// Should only have one row so no executeStep loop
	if (!gDataQuery.executeStep())
		return false;
	loadingProfile->m_sDisplayName =
	  static_cast<const char*>(gDataQuery.getColumn(1));
	loadingProfile->m_sLastUsedHighScoreName =
	  static_cast<const char*>(gDataQuery.getColumn(2));
	loadingProfile->m_sGuid = static_cast<const char*>(gDataQuery.getColumn(3));
	loadingProfile->m_SortOrder =
	  StringToSortOrder(static_cast<const char*>(gDataQuery.getColumn(4)));
	loadingProfile->m_LastDifficulty =
	  static_cast<Difficulty>(static_cast<int>(gDataQuery.getColumn(5)));
	loadingProfile->m_LastStepsType = GAMEMAN->StringToStepsType(
	  static_cast<const char*>(gDataQuery.getColumn(6)));

	string song = static_cast<const char*>(gDataQuery.getColumn(7));
	if (!song.empty())
		loadingProfile->m_lastSong.LoadFromString(song.c_str());
	loadingProfile->m_iCurrentCombo = gDataQuery.getColumn(8);
	loadingProfile->m_iTotalSessions = gDataQuery.getColumn(9);
	loadingProfile->m_iTotalSessionSeconds = gDataQuery.getColumn(10);
	loadingProfile->m_iTotalGameplaySeconds = gDataQuery.getColumn(11);
	loadingProfile->m_LastPlayedDate.FromString(
	  static_cast<const char*>(gDataQuery.getColumn(12)));
	loadingProfile->m_iTotalDancePoints = gDataQuery.getColumn(13);
	loadingProfile->m_iNumToasties = gDataQuery.getColumn(14);
	loadingProfile->m_iTotalTapsAndHolds = gDataQuery.getColumn(15);
	loadingProfile->m_iTotalJumps = gDataQuery.getColumn(16);
	loadingProfile->m_iTotalHolds = gDataQuery.getColumn(17);
	loadingProfile->m_iTotalRolls = gDataQuery.getColumn(18);
	loadingProfile->m_iTotalMines = gDataQuery.getColumn(19);
	loadingProfile->m_iTotalHands = gDataQuery.getColumn(20);
	loadingProfile->m_iTotalLifts = gDataQuery.getColumn(21);
	loadingProfile->m_fPlayerRating =
	  static_cast<float>(static_cast<double>(gDataQuery.getColumn(22)));

	SQLite::Statement modifierQuery(*db, "SELECT * FROM defaultmodifiers");
	while (modifierQuery.executeStep()) {
		const char* modifierName = modifierQuery.getColumn(1);
		const char* modifierValue = modifierQuery.getColumn(2);
		loadingProfile->m_sDefaultModifiers[modifierName] = modifierValue;
	}

	SQLite::Statement skillsetsQuery(*db, "SELECT * FROM playerskillsets");
	while (skillsetsQuery.executeStep()) {
		int skillsetNum = skillsetsQuery.getColumn(1);
		if (skillsetNum == -1)
			skillsetNum = 0;
		auto skillsetValue =
		  static_cast<float>(static_cast<double>(skillsetsQuery.getColumn(2)));
		loadingProfile->m_fPlayerSkillsets[skillsetNum] = skillsetValue;
	}

	SQLite::Statement userTableQuery(*db, "SELECT * FROM usertable");

	auto* L = LUA->Get();

	lua_newtable(L);

	// Only string values and no nested table support (TODO)
	while (userTableQuery.executeStep()) {
		const char* key = userTableQuery.getColumn(0);
		const char* value = userTableQuery.getColumn(1);
		lua_pushstring(L, key);	  // push key
		lua_pushstring(L, value); // push value
		lua_settable(L, -3);
	}

	loadingProfile->m_UserTable.SetFromStack(L);
	LUA->Release(L);

	return true;
}
void
DBProfile::LoadFavourites(SQLite::Database* db)
{
	SQLite::Statement query(*db,
							"SELECT chartkeys.chartkey FROM favourites INNER "
							"JOIN chartkeys ON favourites.chartkeyid = "
							"chartkeys.id");
	while (query.executeStep()) {
		const char* key = query.getColumn(0);
		loadingProfile->FavoritedCharts.emplace(key);
	}
	SONGMAN->SetFavoritedStatus(loadingProfile->FavoritedCharts);
	SongManager::MakePlaylistFromFavorites(loadingProfile->FavoritedCharts);
}

void
DBProfile::LoadPlayLists(SQLite::Database* db)
{
	// First load playlists
	SQLite::Statement query(
	  *db,
	  "SELECT playlists.name, chartkeys.chartkey, "
	  "charts.difficulty, songs.song, songs.pack, chartplaylists.rate "
	  "FROM chartplaylists INNER JOIN playlists ON chartplaylists.playlistid = "
	  "playlists.id "
	  "INNER JOIN charts ON charts.id = chartplaylists.chartid "
	  "INNER JOIN songs ON songs.id = charts.songid "
	  "INNER JOIN chartkeys ON charts.chartkeyid = chartkeys.id "
	  "ORDER BY playlists.name, chartkeys.chartkey, chartplaylists.rate");
	auto& pls = loadingProfile->allplaylists;

	Playlist* tmp = nullptr;

	// Read one row
	if (query.executeStep()) {
		tmp = new Playlist;
		tmp->name = static_cast<const char*>(query.getColumn(0));

		// Load chart
		Chart ch;
		ch.key = static_cast<const char*>(query.getColumn(1));
		ch.lastdiff =
		  static_cast<Difficulty>(static_cast<int>(query.getColumn(2)));
		ch.lastsong = static_cast<const char*>(query.getColumn(3));
		ch.lastpack = static_cast<const char*>(query.getColumn(4));
		ch.rate = static_cast<float>(static_cast<double>(query.getColumn(5)));

		// check if this chart is loaded and overwrite any last-seen values with
		// updated ones
		ch.FromKey(ch.key);

		// Add chart to playlist
		tmp->chartlist.emplace_back(ch);
	} else // Return if there are no palylists(There wont be course runs either)
		return;
	// Read the rest
	while (query.executeStep()) {
		const char* curName = query.getColumn(0);
		const char* key = query.getColumn(1);

		if (curName != tmp->name) {
			// If the playlist changed add it and start a new one
			pls.emplace(tmp->name, *tmp);
			delete tmp;
			tmp = new Playlist;
			tmp->name = curName;
		}

		// Load the chart
		Chart ch;

		ch.lastdiff =
		  static_cast<Difficulty>(static_cast<int>(query.getColumn(2)));
		ch.lastsong = static_cast<const char*>(query.getColumn(3));
		ch.lastpack = static_cast<const char*>(query.getColumn(4));
		ch.rate = static_cast<float>(static_cast<double>(query.getColumn(5)));
		ch.key = key;

		// check if this chart is loaded and overwrite any last-seen values with
		// updated ones
		ch.FromKey(ch.key);

		// Add chart to playlist
		tmp->chartlist.emplace_back(ch);
	}

	pls.emplace(tmp->name, *tmp);
	delete tmp;
	SONGMAN->activeplaylist = tmp->name;
	// Now read courseruns

	SQLite::Statement courseRunsQuery(
	  *db,
	  "SELECT runs.scorekey, courseruns.id, playlists.name "
	  "FROM runs INNER JOIN courseruns ON courseruns.id = runs.courserunid "
	  "INNER JOIN playlists ON playlists.id = courseruns.playlistid "
	  "ORDER BY runs.scorekey, courseruns.id, playlists.name");

	string lastPlayListName;
	int lastCourseRunID = 0;
	std::vector<string> tmpCourseRun;

	// Read one row
	if (courseRunsQuery.executeStep()) {

		const char* curScoreKey = query.getColumn(0);
		lastCourseRunID = query.getColumn(1);
		lastPlayListName = static_cast<const char*>(query.getColumn(2));

		tmpCourseRun.emplace_back(curScoreKey);

	} else
		return;

	// Read the rest
	while (query.executeStep()) {
		const char* curScoreKey = query.getColumn(0);
		int curCourseRunID = query.getColumn(1);

		if (lastCourseRunID != curCourseRunID) {
			// If the courserun changed add it and start a new one
			pls[lastPlayListName].courseruns.emplace_back(tmpCourseRun);
			tmpCourseRun.clear();
			lastPlayListName = static_cast<const char*>(query.getColumn(2));
			lastCourseRunID = curCourseRunID;
		}

		tmpCourseRun.emplace_back(curScoreKey);
	}
	pls[lastPlayListName].courseruns.emplace_back(tmpCourseRun);
	tmpCourseRun.clear();
}

void
DBProfile::LoadPlayerScores(SQLite::Database* db)
{
	SQLite::Statement query(
	  *db,
	  "SELECT chartkeys.chartkey, "
	  "songs.song, songs.pack, charts.difficulty, "
	  "scoresatrates.rate, "
	  "scorekeys.scorekey, scores.calcversion, "
	  "scores.grade, scores.wifescore, scores.ssrnormpercent, "
	  "scores.judgescale, scores.nochordcohesion, scores.etternavalid, "
	  "scores.playedseconds, scores.maxcombo, scores.modifiers, "
	  "scores.datetime, "
	  "scores.hitmine, scores.avoidmine, scores.miss, scores.w5, scores.w4, "
	  "scores.w3, "
	  "scores.w2, scores.w1, scores.letgoholds, scores.heldholds, "
	  "scores.missedholds, "
	  "scores.overall, scores.stream, scores.jumpstream, "
	  "scores.handstream, stamina, scores.jackspeed, "
	  "scores.jackstamina, scores.technical, scores.brittle, scores.weak FROM "
	  "scores "
	  "INNER JOIN scorekeys ON scores.scorekeyid = scorekeys.id "
	  "INNER JOIN scoresatrates ON scoresatrates.id = scores.scoresatrateid "
	  "INNER JOIN charts ON charts.id=scoresatrates.chartid "
	  "INNER JOIN songs ON songs.id = charts.songid "
	  "INNER JOIN chartkeys ON chartkeys.id=charts.chartkeyid "
	  "ORDER BY chartkeys.id, charts.id, scoresatrates.id");

	auto& scores = *(SCOREMAN->GetProfileScores());

	string curCK;

	while (query.executeStep()) {
		const string key = static_cast<const char*>(query.getColumn(0));
		if (key != curCK) {
			// Per Chart
			curCK = key;
			scores[key].ch.key = key;
			scores[key].ch.lastsong =
			  static_cast<const char*>(query.getColumn(1));
			scores[key].ch.lastpack =
			  static_cast<const char*>(query.getColumn(2));
			scores[key].ch.lastdiff =
			  static_cast<Difficulty>(static_cast<int>(query.getColumn(3)));
		}

		int rate = query.getColumn(4);

		// Per Score
		string ScoreKey = query.getColumn(5);
		auto& hs = scores[key].ScoresByRate[rate].scores[ScoreKey];
		hs.SetSSRCalcVersion(query.getColumn(6));
		hs.SetGrade(static_cast<Grade>(static_cast<int>(query.getColumn(7))));
		hs.SetWifeScore(
		  static_cast<float>(static_cast<double>(query.getColumn(8))));
		hs.SetSSRNormPercent(
		  static_cast<float>(static_cast<double>(query.getColumn(9))));
		hs.SetMusicRate(scores[key].KeyToRate(rate));
		hs.SetJudgeScale(
		  static_cast<float>(static_cast<double>(query.getColumn(10))));
		hs.SetChordCohesion(static_cast<int>(query.getColumn(11)) != 0);
		hs.SetEtternaValid(static_cast<int>(query.getColumn(12)) != 0);
		hs.SetChartKey(key);
		hs.SetScoreKey(ScoreKey);
		hs.SetPlayedSeconds(
		  static_cast<float>(static_cast<double>(query.getColumn(13))));
		hs.SetMaxCombo(query.getColumn(14));
		hs.SetModifiers(query.getColumn(15));
		DateTime d;
		d.FromString(static_cast<const char*>(query.getColumn(16)));
		hs.SetDateTime(d);

		auto index = 17;

		FOREACH_ENUM(TapNoteScore, tns)
		if (tns != TNS_None && tns != TNS_CheckpointMiss &&
			tns != TNS_CheckpointHit)
			hs.SetTapNoteScore(tns, query.getColumn(index++));

		FOREACH_ENUM(HoldNoteScore, hns)
		if (hns != HNS_None)
			hs.SetHoldNoteScore(hns, query.getColumn(index++));

		if (hs.GetWifeScore() > 0.f) {
			FOREACH_ENUM(Skillset, ss)
			hs.SetSkillsetSSR(ss,
							  static_cast<float>(
								static_cast<double>(query.getColumn(index++))));
		}
		hs.SetValidationKey(ValidationKey_Brittle,
							static_cast<const char*>(query.getColumn(index++)));
		hs.SetValidationKey(ValidationKey_Weak,
							static_cast<const char*>(query.getColumn(index++)));

		if (hs.GetScoreKey().empty())
			hs.SetScoreKey("S" + BinaryToHex(CryptManager::GetSHA1ForString(
								   hs.GetDateTime().GetString())));

		// Validate input.
		hs.SetGrade(std::clamp(hs.GetGrade(), Grade_Tier01, Grade_Failed));

		// Set any pb
		if (scores[key].ScoresByRate[rate].PBptr == nullptr)
			scores[key].ScoresByRate[rate].PBptr =
			  &scores[key].ScoresByRate[rate].scores.find(ScoreKey)->second;
		else {
			// update pb if a better score is found
			if (PREFSMAN->m_bSortBySSRNorm) {

				if (scores[key].ScoresByRate[rate].PBptr->GetSSRNormPercent() <
					hs.GetSSRNormPercent())
					scores[key].ScoresByRate[rate].PBptr =
					  &scores[key]
						 .ScoresByRate[rate]
						 .scores.find(ScoreKey)
						 ->second;
			} else {

				if (scores[key].ScoresByRate[rate].PBptr->GetWifeScore() <
					hs.GetWifeScore())
					scores[key].ScoresByRate[rate].PBptr =
					  &scores[key]
						 .ScoresByRate[rate]
						 .scores.find(ScoreKey)
						 ->second;
			}
		};

		scores[key].ScoresByRate[rate].bestGrade =
		  std::min(hs.GetWifeGrade(), scores[key].ScoresByRate[rate].bestGrade);

		// Very awkward, need to figure this out better so there isn't
		// unnecessary redundancy between loading and adding
		SCOREMAN->RegisterScore(&hs);
		SCOREMAN->AddToKeyedIndex(&hs);

		scores[key].bestGrade = std::min(
		  scores[key].ScoresByRate[rate].bestGrade, scores[key].bestGrade);
	}
}
void
DBProfile::LoadPermaMirrors(SQLite::Database* db)
{
	SQLite::Statement query(
	  *db,
	  "SELECT chartkeys.chartkey FROM permamirrors INNER "
	  "JOIN chartkeys ON permamirrors.chartkeyid = chartkeys.id");
	while (query.executeStep()) {
		const char* key = query.getColumn(0);
		loadingProfile->PermaMirrorCharts.emplace(key);
	}
	SONGMAN->SetPermaMirroredStatus(loadingProfile->PermaMirrorCharts);
}
void
DBProfile::LoadScoreGoals(SQLite::Database* db)
{
	SQLite::Statement query(
	  *db,
	  "SELECT chartkeys.chartkey, scoregoals.rate, scoregoals.priority, "
	  "scoregoals.percent, "
	  "scoregoals.timeassigned, scoregoals.timeachieved, scoregoals.scorekey, "
	  "scoregoals.comment FROM scoregoals INNER JOIN chartkeys ON "
	  "scoregoals.chartkeyid = chartkeys.id ORDER BY chartkeys.chartkey ASC");

	string ck;
	while (query.executeStep()) {
		ck = static_cast<const char*>(query.getColumn(0));
		// Load the scoregoal
		ScoreGoal sg;
		sg.rate = static_cast<float>(static_cast<double>(query.getColumn(1)));
		sg.priority = query.getColumn(2);
		sg.percent =
		  static_cast<float>(static_cast<double>(query.getColumn(3)));
		sg.timeassigned.FromString(
		  static_cast<const char*>(query.getColumn(4)));
		sg.timeachieved.FromString(
		  static_cast<const char*>(query.getColumn(5)));
		sg.scorekey = static_cast<const char*>(query.getColumn(6));
		sg.comment = static_cast<const char*>(query.getColumn(7));
		// Add it to the GoalsForAChart goalmap[chart]
		loadingProfile->goalmap[ck].Add(sg);
	}
}

ProfileLoadResult
DBProfile::SaveDBToDir(const string& dir,
					   const Profile* profile,
					   DBProfileMode mode) const
{
	string filename;
	switch (mode) {
		case WriteOnlyWebExport:
			filename = FILEMAN->ResolvePath(dir) + WRITE_ONLY_PROFILE_DB;
			break;
		case LocalWithReplayData:
		case LocalWithoutReplayData:
			filename = FILEMAN->ResolvePath(dir) + PROFILE_DB;
			break;
		default:
			break;
	}
	SQLite::Database* db = nullptr;
	try {
		// Open a database file
		db = new SQLite::Database(filename,
								  SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
	} catch (std::exception&) {
		return ProfileLoadResult_FailedNoProfile;
	}
	try {
		// Use the db
		// Begin transaction
		SQLite::Transaction transaction(*db);
		// We need to initialize these tables here
		db->exec("CREATE TABLE IF NOT EXISTS chartkeys (id INTEGER PRIMARY "
				 "KEY, chartkey TEXT)");
		db->exec("DROP TABLE IF EXISTS skillsets");
		db->exec("CREATE TABLE skillsets (skillsetnum INTEGER PRIMARY KEY, "
				 "skillset TEXT)");
		FOREACH_ENUM(Skillset, ss)
		db->exec("INSERT INTO skillsets VALUES (" +
				 std::to_string(static_cast<int>(ss)) + ", \"" +
				 SkillsetToString(ss) + "\")");
		if (mode == WriteOnlyWebExport)
			SaveGeneralData(db, profile);
		SaveFavourites(db, profile);
		SavePermaMirrors(db, profile);
		SaveScoreGoals(db, profile);
		// Make sure playlists are loaded after playerscores
		SavePlayerScores(db, profile, mode);
		SavePlayLists(db, profile);

		db->exec("CREATE INDEX IF NOT EXISTS idx_scoregoals "
				 "ON scoregoals(chartkeyid, id)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_scoresatrates "
				 "ON scoresatrates(chartid, id)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_favs "
				 "ON favourites(chartkeyid, id)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_permamirrors "
				 "ON permamirrors(chartkeyid, id)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_playlists "
				 "ON playlists(id, name)");
		db->exec("CREATE INDEX IF NOT EXISTS  idx_chartplaylists "
				 "ON chartplaylists(playlistid, chartid, rate)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_charts "
				 "ON charts(chartkeyid, songid, difficulty, id)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_chartkeys1 "
				 "ON chartkeys(id, chartkey)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_chartkeys2 "
				 "ON chartkeys(chartkey, id)");
		db->exec("CREATE INDEX IF NOT EXISTS idx_songs "
				 "ON songs(song, pack, id)");
		transaction.commit();
	} catch (std::exception&) {
		return ProfileLoadResult_FailedTampered;
	}
	delete db;
	return ProfileLoadResult_Success;
}

void
DBProfile::SaveFavourites(SQLite::Database* db, const Profile* profile)
{

	db->exec("DROP TABLE IF EXISTS favourites");
	db->exec("CREATE TABLE favourites (id INTEGER PRIMARY KEY, chartkeyid "
			 "INTEGER, CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) "
			 "REFERENCES chartkeys(id))");
	if (!profile->FavoritedCharts.empty()) {
		for (auto& ck : profile->FavoritedCharts) {
			SQLite::Statement insertFav(
			  *db, "INSERT INTO favourites VALUES (NULL, ?)");
			insertFav.bind(1, FindOrCreateChartKey(db, ck));
			insertFav.exec();
		}
	}
}

void
DBProfile::SaveGeneralData(SQLite::Database* db, const Profile* profile)
{

	db->exec("DROP TABLE IF EXISTS generaldata");
	db->exec("CREATE TABLE generaldata (id INTEGER PRIMARY KEY, "
			 "displayname TEXT, guid TEXT, sortorder TEXT, "
			 "lastdiff INTEGER, laststeps TEXT, lastsong TEXT, totalsessions "
			 "INTEGER, totalsessionseconds INTEGER, "
			 "totalgameplayseconds INTEGER, lastplayedmachineguid TEXT, "
			 "lastplayeddate DATE, "
			 "totaldancepoints INTEGER, numtoasties INTEGER, totaltapsandholds "
			 "INTEGER, "
			 "totaljumps INTEGER, totalholds INTEGER, totalrolls INTEGER, "
			 "totalmines INTEGER, "
			 "totalhands INTEGER, totallifts INTEGER, rating DOUBLE, "
			 "totalsongsplayed INTEGER)");

	SQLite::Statement insertGData(
	  *db,
	  "INSERT INTO generaldata VALUES (NULL, ?,"
	  "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
	insertGData.bind(1, profile->GetDisplayNameOrHighScoreName());
	insertGData.bind(2, profile->m_sGuid);
	insertGData.bind(3, SortOrderToString(profile->m_SortOrder));
	insertGData.bind(4, profile->m_LastDifficulty);
	insertGData.bind(
	  5,
	  ((profile->m_LastStepsType < NUM_StepsType)
		 ? GAMEMAN->GetStepsTypeInfo(profile->m_LastStepsType).szName
		 : ""));
	insertGData.bind(6, profile->m_lastSong.ToString());
	insertGData.bind(7, profile->m_iTotalSessions);
	insertGData.bind(8, profile->m_iTotalSessionSeconds);
	insertGData.bind(9, profile->m_iTotalGameplaySeconds);
	insertGData.bind(10, profile->m_sLastPlayedMachineGuid);
	insertGData.bind(11, profile->m_LastPlayedDate.GetString());
	insertGData.bind(12, profile->m_iTotalDancePoints);
	insertGData.bind(13, profile->m_iNumToasties);
	insertGData.bind(14, profile->m_iTotalTapsAndHolds);
	insertGData.bind(15, profile->m_iTotalJumps);
	insertGData.bind(16, profile->m_iTotalHolds);
	insertGData.bind(17, profile->m_iTotalRolls);
	insertGData.bind(18, profile->m_iTotalMines);
	insertGData.bind(19, profile->m_iTotalHands);
	insertGData.bind(20, profile->m_iTotalLifts);
	insertGData.bind(21, profile->m_fPlayerRating);
	insertGData.bind(22, profile->m_iNumTotalSongsPlayed);
	insertGData.exec();
	db->exec("DROP TABLE IF EXISTS defaultmodifiers");
	db->exec("CREATE TABLE defaultmodifiers (id INTEGER PRIMARY KEY, "
			 "name TEXT, value TEXT)");

	for (const auto& it : profile->m_sDefaultModifiers) {
		db->exec("INSERT INTO defaultmodifiers VALUES (NULL, \"" + it.first +
				 "\", \"" + it.second + "\")");
	}

	db->exec("DROP TABLE IF EXISTS playerskillsets");
	db->exec("CREATE TABLE playerskillsets (id INTEGER PRIMARY KEY, "
			 "skillsetnum INTEGER, value DOUBLE, "
			 "CONSTRAINT fk_skillsetnum FOREIGN KEY (skillsetnum) REFERENCES "
			 "skillsets(skillsetnum))");

	FOREACH_ENUM(Skillset, ss)
	db->exec("INSERT INTO playerskillsets VALUES (NULL, " + std::to_string(ss) +
			 ", " + std::to_string(profile->m_fPlayerSkillsets[ss]) + ")");

	db->exec("DROP TABLE IF EXISTS usertable");
	db->exec("CREATE TABLE usertable (id INTEGER PRIMARY KEY, "
			 "key TEXT, value TEXT)");
	/*	TODO:UserTables
	if (profile->m_UserTable.IsSet())
	{
		Lua *L = LUA->Get();
		profile->m_UserTable.PushSelf(L);
		//XNode* pUserTable = XmlFileUtil::XNodeFromTable(L);
		//TODO
		LUA->Release(L);
	}
	*/
}

void
DBProfile::MoveBackupToDir(const string& sFromDir,
						   const string& sToDir,
						   DBProfileMode mode)
{
	string filename;
	switch (mode) {
		case WriteOnlyWebExport:
			filename = WRITE_ONLY_PROFILE_DB;
			break;
		case LocalWithReplayData:
		case LocalWithoutReplayData:
			filename = PROFILE_DB;
			break;
		default:
			break;
	}

	if (FILEMAN->IsAFile(sFromDir + filename))
		FILEMAN->Move(sFromDir + filename, sToDir + filename);
}

void
DBProfile::SavePermaMirrors(SQLite::Database* db, const Profile* profile)
{

	db->exec("DROP TABLE IF EXISTS permamirrors");
	db->exec("CREATE TABLE permamirrors (id INTEGER PRIMARY KEY, chartkeyid "
			 "INTEGER, CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) "
			 "REFERENCES chartkeys(id))");

	if (!profile->PermaMirrorCharts.empty()) {
		for (auto& it : profile->PermaMirrorCharts) {
			const auto chID = FindOrCreateChartKey(db, it);
			db->exec("INSERT INTO permamirrors VALUES (NULL, " +
					 std::to_string(chID) + ")");
		}
	}
}

void
DBProfile::SaveScoreGoals(SQLite::Database* db, const Profile* profile)
{

	db->exec("DROP TABLE IF EXISTS scoregoals");
	db->exec(
	  "CREATE TABLE scoregoals (id INTEGER PRIMARY KEY, chartkeyid INTEGER, "
	  "rate FLOAT, priority INTEGER, percent FLOAT, timeassigned DATE, "
	  "timeachieved DATE, "
	  "scorekey TEXT, comment TEXT, CONSTRAINT fk_chartkeyid FOREIGN KEY "
	  "(chartkeyid) REFERENCES chartkeys(id))");

	if (!profile->goalmap.empty()) {
		for (const auto& i : profile->goalmap) {
			const auto& cg = i.second;
			if (cg.goals.empty())
				continue;
			const auto chID = FindOrCreateChartKey(db, cg.goals[0].chartkey);
			for (const auto& sg : cg.goals) {
				SQLite::Statement insertScoreGoal(*db,
												  "INSERT INTO scoregoals "
												  "VALUES (NULL, ?, ?, ?, ?, "
												  "?, ?, ?, ?)");
				insertScoreGoal.bind(1, chID);
				insertScoreGoal.bind(2, sg.rate);
				insertScoreGoal.bind(3, sg.priority);
				insertScoreGoal.bind(4, sg.percent);
				insertScoreGoal.bind(5, sg.timeassigned.GetString());
				if (sg.achieved) {
					insertScoreGoal.bind(6, sg.timeachieved.GetString());
					insertScoreGoal.bind(7, sg.scorekey);
				} else {
					// bind null values
					insertScoreGoal.bind(6);
					insertScoreGoal.bind(7);
				}
				insertScoreGoal.bind(8, sg.comment);
				insertScoreGoal.exec();
			}
		}
	}
}

void
DBProfile::SavePlayLists(SQLite::Database* db, const Profile* profile)
{
	db->exec("DROP TABLE IF EXISTS playlists");
	db->exec("CREATE TABLE playlists (id INTEGER PRIMARY KEY, name TEXT)");

	db->exec("DROP TABLE IF EXISTS chartplaylists");
	db->exec(
	  "CREATE TABLE chartplaylists (id INTEGER PRIMARY KEY, chartid INTEGER, "
	  "playlistid INTEGER, rate FLOAT, "
	  "CONSTRAINT fk_chartid FOREIGN KEY (chartid) REFERENCES charts(id), "
	  "CONSTRAINT fk_playlistid FOREIGN KEY (playlistid) REFERENCES "
	  "playlists(id))");

	db->exec("DROP TABLE IF EXISTS courseruns");
	db->exec(
	  "CREATE TABLE courseruns (id INTEGER PRIMARY KEY, playlistid INTEGER, "
	  "CONSTRAINT fk_playlistid FOREIGN KEY (playlistid) REFERENCES "
	  "playlists(id))");

	db->exec("DROP TABLE IF EXISTS runs");
	db->exec("CREATE TABLE runs (id INTEGER PRIMARY KEY, scorekey TEXT, "
			 "courserunid INTEGER, "
			 "CONSTRAINT fk_courserunid FOREIGN KEY (courserunid) REFERENCES "
			 "courseruns(id))");

	const auto& pls = profile->allplaylists;
	if (!pls.empty()) {
		for (const auto& pl : pls) {
			if (!pl.first.empty() && pl.first != "Favorites") {
				SQLite::Statement insertPlaylist(
				  *db, "INSERT INTO playlists VALUES (NULL, ?)");
				insertPlaylist.bind(1, (pl.second).name);
				insertPlaylist.exec();
				// db->exec("INSERT INTO playlists VALUES (NULL, \"" +
				// (pl->second).name + "\")");
				auto plID =
				  static_cast<int>(sqlite3_last_insert_rowid(db->getHandle()));
				for (const auto& ch : pl.second.chartlist) {
					auto chartID = FindOrCreateChart(
					  db, ch.key, ch.lastpack, ch.lastsong, ch.lastdiff);
					SQLite::Statement insertChartPlaylist(
					  *db, "INSERT INTO chartplaylists VALUES (NULL, ?, ?, ?)");
					insertChartPlaylist.bind(1, chartID);
					insertChartPlaylist.bind(2, plID);
					insertChartPlaylist.bind(3, ch.rate);
					insertChartPlaylist.exec();
				}

				for (const auto& run : pl.second.courseruns) {
					SQLite::Statement insertCourseRun(
					  *db, "INSERT INTO courseruns VALUES (NULL, ?)");
					insertCourseRun.bind(1, plID);
					insertCourseRun.exec();
					auto courseRunID = static_cast<int>(
					  sqlite3_last_insert_rowid(db->getHandle()));
					for (const auto& sk : run) {
						SQLite::Statement insertRun(
						  *db, "INSERT INTO runs VALUES (NULL, ?, ?)");
						insertRun.bind(1, sk);
						insertRun.bind(2, courseRunID);
						insertRun.exec();
					}
				}
			}
		}
	}
}

void
DBProfile::SavePlayerScores(SQLite::Database* db,
							const Profile* profile,
							DBProfileMode mode)
{
	if (mode != WriteOnlyWebExport) {
		// Separate scorekeys table so we can not drop it when saving, and
		// we can relate offsets/replaydata(Which is saved after each score) to
		// scorekeys
		db->exec("CREATE TABLE IF NOT EXISTS scorekeys (id INTEGER PRIMARY "
				 "KEY, scorekey TEXT)");

		db->exec("DROP TABLE IF EXISTS scores");
		db->exec(
		  "CREATE TABLE IF NOT EXISTS scores (id INTEGER PRIMARY KEY, "
		  "scoresatrateid INTEGER, "
		  "scorekeyid INTEGER, calcversion INT, grade INTEGER, wifescore "
		  "FLOAT, "
		  "ssrnormpercent FLOAT, judgescale FLOAT, nochordcohesion INTEGER, "
		  "etternavalid INTEGER, surviveseconds FLOAT, maxcombo INTEGER, "
		  "modifiers TEXT, datetime DATE, "
		  "hitmine INTEGER, avoidmine INTEGER, miss INTEGER, w5 INTEGER, w4 "
		  "INTEGER, "
		  "w3 INTEGER, w2 INTEGER, w1 INTEGER, letgoholds INTEGER, heldholds "
		  "INTEGER, "
		  "missedholds INTEGER, overall FLOAT, stream FLOAT, jumpstream FLOAT, "
		  "handstream FLOAT, stamina FLOAT, jackspeed FLOAT, "
		  "jackstamina FLOAT, technical FLOAT, brittle TEXT, weak TEXT, "
		  "CONSTRAINT fk_scoresatrateid FOREIGN KEY (scoresatrateid) "
		  "REFERENCES scoresatrates(id), "
		  "CONSTRAINT fk_scorekeyid FOREIGN KEY (scorekeyid) REFERENCES "
		  "scorekeys(id))");

		db->exec("DROP TABLE IF EXISTS scoresatrates");
		db->exec(
		  "CREATE TABLE IF NOT EXISTS scoresatrates (id INTEGER PRIMARY KEY, "
		  "chartid INTEGER, "
		  "rate INTEGER, bestgrade TEXT, pbkey TEXT, "
		  "CONSTRAINT fk_chartid FOREIGN KEY (chartid) REFERENCES charts(id))");

	} else {
		db->exec("DROP TABLE IF EXISTS scoresatrates");
		db->exec(
		  "CREATE TABLE IF NOT EXISTS scoresatrates (id INTEGER PRIMARY KEY, "
		  "chartid INTEGER, "
		  "scorekey TEXT, calcversion INT, grade INTEGER, wifescore FLOAT, "
		  "ssrnormpercent FLOAT, judgescale FLOAT, nochordcohesion INTEGER, "
		  "etternavalid INTEGER, surviveseconds FLOAT, maxcombo INTEGER, "
		  "modifiers TEXT, datetime DATE, "
		  "hitmine INTEGER, avoidmine INTEGER, miss INTEGER, w5 INTEGER, w4 "
		  "INTEGER, "
		  "w3 INTEGER, w2 INTEGER, w1 INTEGER, letgoholds INTEGER, heldholds "
		  "INTEGER, "
		  "missedholds INTEGER, overall FLOAT, stream FLOAT, jumpstream FLOAT, "
		  "handstream FLOAT, stamina FLOAT, jackspeed FLOAT, "
		  "jackstamina FLOAT, technical FLOAT, brittle TEXT, weak TEXT, rate "
		  "INTEGER, "
		  "CONSTRAINT fk_chartid FOREIGN KEY (chartid) REFERENCES charts(id))");
	}

	db->exec("DROP TABLE IF EXISTS charts");
	db->exec("CREATE TABLE IF NOT EXISTS charts (id INTEGER PRIMARY KEY, "
			 "chartkeyid INTEGER, "
			 "songid INTEGER, difficulty INTEGER, "
			 "CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) REFERENCES "
			 "chartkeys(id), "
			 "CONSTRAINT fk_songid FOREIGN KEY (songid) REFERENCES songs(id))");

	db->exec("DROP TABLE IF EXISTS songs");
	db->exec("CREATE TABLE IF NOT EXISTS songs (id INTEGER PRIMARY KEY, "
			 "pack TEXT, song TEXT)");

	if (mode != WriteOnlyWebExport) {
		db->exec("CREATE TABLE IF NOT EXISTS offsets (id INTEGER PRIMARY KEY, "
				 "row INTEGER, offset FLOAT, scorekeyid INTEGER, "
				 "CONSTRAINT fk_scorekeyid FOREIGN KEY (scorekeyid) REFERENCES "
				 "scorekeys(id))");
	}

	auto& pScores = *SCOREMAN->GetProfileScores();
	for (auto& chartPair : pScores) {
		// First is ckey and two is ScoresForChart
		auto ch = ((chartPair.second).ch);
		ch.FromKey(chartPair.first);

		auto chartKeyID = FindOrCreateChartKey(db, ch.key);
		auto songID = FindOrCreateSong(db, ch.lastpack, ch.lastsong);
		SQLite::Statement insertChart(
		  *db, "INSERT INTO charts VALUES (NULL, ?, ?, ?)");
		insertChart.bind(1, chartKeyID);
		insertChart.bind(2, songID);
		insertChart.bind(3, ch.lastdiff);
		insertChart.exec();
		auto chartID =
		  static_cast<int>(sqlite3_last_insert_rowid(db->getHandle()));

		// Add scores per rate
		for (auto& ratePair : chartPair.second.ScoresByRate) {
			// first is rate int and second is ScoresAtRate
			auto rate = ratePair.first;
			int scoresAtRateID = 0;
			if (mode != WriteOnlyWebExport) {
				SQLite::Statement insertScoresAtRate(
				  *db, "INSERT INTO scoresatrates VALUES (NULL, ?, ?, ?, ?)");
				insertScoresAtRate.bind(1, chartID);
				insertScoresAtRate.bind(2, rate);
				insertScoresAtRate.bind(
				  3, GradeToString(ratePair.second.bestGrade));
				insertScoresAtRate.bind(4,
										ratePair.second.PBptr->GetScoreKey());
				insertScoresAtRate.exec();
				scoresAtRateID =
				  static_cast<int>(sqlite3_last_insert_rowid(db->getHandle()));
			}
			for (auto& i : ratePair.second.scores) {
				if (mode != WriteOnlyWebExport ||
					i.second.GetScoreKey() ==
					  ratePair.second.PBptr->GetScoreKey()) {
					// prune out sufficiently low scores
					if (i.second.GetWifeScore() > SCOREMAN->minpercent) {
						auto* const hs = &(i.second);
						// Add scores
						SQLite::Statement* insertScore = nullptr;
						int scorekeyID = 0;
						if (mode != WriteOnlyWebExport) {
							insertScore = new SQLite::Statement(
							  *db,
							  "INSERT INTO scores VALUES (NULL, ?, ?, ?, ?, ?, "
							  "?, ?, "
							  "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
							  "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
							insertScore->bind(1, scoresAtRateID);
							scorekeyID = FindOrCreateScoreKey(
							  db,
							  (hs->GetScoreKey().empty()
								 ? "S" +
									 BinaryToHex(CryptManager::GetSHA1ForString(
									   hs->GetDateTime().GetString()))
								 : hs->GetScoreKey()));
							insertScore->bind(2, scorekeyID);
						} else {
							insertScore = new SQLite::Statement(
							  *db,
							  "INSERT INTO scoresatrates VALUES (NULL, ?, ?, "
							  "?, ?, ?, ?, ?, "
							  "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
							  "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
							insertScore->bind(1, chartID);
							insertScore->bind(
							  2,
							  (hs->GetScoreKey().empty()
								 ? "S" +
									 BinaryToHex(CryptManager::GetSHA1ForString(
									   hs->GetDateTime().GetString()))
								 : hs->GetScoreKey()));
						}
						insertScore->bind(3, hs->GetSSRCalcVersion());
						insertScore->bind(4, hs->GetWifeGrade());
						insertScore->bind(5, hs->GetWifeScore());
						insertScore->bind(6, hs->GetSSRNormPercent());
						insertScore->bind(7, hs->GetJudgeScale());
						insertScore->bind(8, static_cast<int>(hs->GetChordCohesion()));
						insertScore->bind(9, static_cast<int>(hs->GetEtternaValid()));
						insertScore->bind(10, hs->GetPlayedSeconds());
						insertScore->bind(11, hs->GetMaxCombo());
						insertScore->bind(12, hs->GetModifiers());
						insertScore->bind(13, hs->GetDateTime().GetString());

						auto index = 14;
						FOREACH_ENUM(TapNoteScore, tns)
						if (tns != TNS_None && tns != TNS_CheckpointMiss &&
							tns != TNS_CheckpointHit)
							insertScore->bind(index++,
											  hs->GetTapNoteScore(tns));

						FOREACH_ENUM(HoldNoteScore, hns)
						if (hns != HNS_None)
							insertScore->bind(index++,
											  hs->GetHoldNoteScore(hns));

						if (hs->GetWifeScore() > 0.f &&
							hs->GetWifeGrade() != Grade_Failed &&
							hs->GetSkillsetSSR(Skill_Overall) > 0.f)
							FOREACH_ENUM(Skillset, ss)
						insertScore->bind(index++, hs->GetSkillsetSSR(ss));
						else FOREACH_ENUM(Skillset, ss)
						  insertScore->bind(index++, 0);

						insertScore->bind(
						  index++, hs->GetValidationKey(ValidationKey_Brittle));
						insertScore->bind(
						  index++, hs->GetValidationKey(ValidationKey_Weak));

						if (mode == WriteOnlyWebExport)
							insertScore->bind(index++, rate);

						insertScore->exec();
						delete insertScore;
						if (mode == WriteOnlyWebExport)
							break;
						if (mode == LocalWithReplayData) {
							try {
								// Save Replay Data
								if (hs->LoadReplayData()) {
									const auto& offsets = hs->GetOffsetVector();
									const auto& rows = hs->GetNoteRowVector();
									unsigned int idx =
									  !rows.empty() ? rows.size() - 1 : 0U;
									// loop for writing both vectors side by
									// side
									for (unsigned int offsetIndex = 0;
										 offsetIndex < idx;
										 offsetIndex++) {
										SQLite::Statement insertOffset(
										  *db,
										  "INSERT INTO offsets VALUES (NULL, "
										  "?, ?, ?)");
										insertOffset.bind(1, rows[offsetIndex]);
										insertOffset.bind(2,
														  offsets[offsetIndex]);
										insertOffset.bind(3, scorekeyID);
										insertOffset.exec();
									}
								}
							} catch (std::exception&) { // No replay data for
													   // this score }
								hs->UnloadReplayData();
							}
						}
					}
				}
			}
		}
	}
}
int
DBProfile::GetChartKeyID(SQLite::Database* db, const string& key)
{
	SQLite::Statement query(*db, "SELECT * FROM chartkeys WHERE chartkey=?");
	query.bind(1, key);
	if (!query.executeStep())
		return 0;
	return query.getColumn(0);
}

string
DBProfile::GetChartKeyByID(SQLite::Database* db, int id)
{
	SQLite::Statement query(*db, "SELECT * FROM chartkeys WHERE id=?");
	query.bind(1, id);
	if (!query.executeStep())
		return "";
	return static_cast<const char*>(query.getColumn(1));
}

int
DBProfile::FindOrCreateChartKey(SQLite::Database* db, const string& key)
{
	const auto exists = GetChartKeyID(db, key);
	if (exists != 0)
		return exists;
	db->exec("INSERT INTO chartkeys VALUES (NULL, \"" + key + "\")");
	return static_cast<int>(sqlite3_last_insert_rowid(db->getHandle()));
}

int
DBProfile::FindOrCreateSong(SQLite::Database* db, const string& pack, const string& song)
{
	SQLite::Statement query(
	  *db, "SELECT songs.id FROM songs WHERE song=? AND pack =?");
	query.bind(1, song);
	query.bind(2, pack);
	if (!query.executeStep()) {
		SQLite::Statement insertSong(*db,
									 "INSERT INTO songs VALUES (NULL, ?, ?)");

		insertSong.bind(1, pack);
		insertSong.bind(2, song);
		insertSong.exec();
		return static_cast<int>(sqlite3_last_insert_rowid(db->getHandle()));
	}
	return query.getColumn(0);
}

int
DBProfile::FindOrCreateChart(SQLite::Database* db,
							 const string& chartkey,
							 const string& pack,
							 const string& song,
							 Difficulty diff)
{
	const auto chartKeyID = FindOrCreateChartKey(db, chartkey);
	const auto songID = FindOrCreateSong(db, pack, song);
	// Find or create chart now
	// Check if chart already exists
	SQLite::Statement query(
	  *db,
	  "SELECT * FROM charts WHERE chartkeyid=? AND songid=? AND difficulty=?");
	query.bind(1, chartKeyID);
	query.bind(2, songID);
	query.bind(3, diff);
	// if not
	if (!query.executeStep()) {
		// insert the chart
		SQLite::Statement insertChart(
		  *db, "INSERT INTO charts VALUES (NULL, ?, ?, ?)");
		insertChart.bind(1, chartKeyID);
		insertChart.bind(2, songID);
		insertChart.bind(3, diff);
		insertChart.exec();
		return static_cast<int>(sqlite3_last_insert_rowid(db->getHandle()));
	}

	return query.getColumn(0);
}

int
DBProfile::GetScoreKeyID(SQLite::Database* db, const string& key)
{
	SQLite::Statement query(*db, "SELECT * FROM scorekeys WHERE scorekey=?");
	query.bind(1, key);
	if (!query.executeStep())
		return 0;
	return query.getColumn(0);
}

int
DBProfile::FindOrCreateScoreKey(SQLite::Database* db, const string& key)
{
	const auto exists = GetScoreKeyID(db, key);
	if (exists != 0)
		return exists;
	db->exec("INSERT INTO scorekeys VALUES (NULL, \"" + key + "\")");
	return static_cast<int>(sqlite3_last_insert_rowid(db->getHandle()));
}

bool
DBProfile::WriteReplayData(const HighScore* hs)
{
	// we should not be writing replay data to the database.
	return false;
	/*
	const auto profiledir =
	  PROFILEMAN->GetProfileDir(ProfileSlot_Player1).substr(1);
	const auto filename = profiledir + PROFILE_DB;

	SQLite::Database* db = nullptr;
	try {
		// Open a database file
		db = new SQLite::Database(filename,
								  SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
	} catch (std::exception&) {
		return ProfileLoadResult_FailedNoProfile != 0u;
	}
	try {
		// Use the db
		// Begin transaction
		SQLite::Transaction transaction(*db);
		const auto& rows = hs->GetNoteRowVector();
		const auto& offsets = hs->GetOffsetVector();
		const unsigned int idx = !rows.empty() ? rows.size() - 1 : 0U;
		// loop for writing both vectors side by side
		const auto scoreKeyID = FindOrCreateScoreKey(db, hs->GetScoreKey());
		for (unsigned int i = 0; i < idx; i++) {
			SQLite::Statement insertOffset(
			  *db, "INSERT INTO offsets VALUES (NULL, ?, ?, ?)");
			insertOffset.bind(1, rows[i]);
			insertOffset.bind(2, offsets[i]);
			insertOffset.bind(3, scoreKeyID);
			insertOffset.exec();
		}
		transaction.commit();
	} catch (std::exception&) {
		return ProfileLoadResult_FailedTampered != 0;
	}
	delete db;
	return true;
	*/
}
