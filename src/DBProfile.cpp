
#include "global.h"
#include "Profile.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ProfileManager.h"
#include "NoteData.h"
#include "DBProfile.h"
#include "RageFile.h"
#include "RageFileDriverDeflate.h"
#include "GameState.h"
#include "GameManager.h"
#include "LuaManager.h"
#include "NoteData.h"
#include "RageFileManager.h"

#include "ScoreManager.h"
#include "CryptManager.h"
#include "Song.h"
#include "SongManager.h"
#include "Steps.h"
#include "sqlite3.h"
#include "RageFileManager.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

const RString PROFILE_DB= "profile.db";

ProfileLoadResult DBProfile::LoadDBFromDir(RString dir, Profile* profile)
{
	loadingProfile = profile;
	return LoadDBFromDir(dir);
}

ProfileLoadResult DBProfile::LoadDBFromDir(RString dir)
{
	
	SQLite::Database *db;
	try {
		// Open a database file
		db = new SQLite::Database(FILEMAN->ResolvePath(dir) + PROFILE_DB, SQLite::OPEN_READWRITE);
	}catch (std::exception& e)
	{
		return ProfileLoadResult_FailedNoProfile;
	}
	try {
		// Use the db
		if(!LoadGeneralData(db))
			return ProfileLoadResult_FailedTampered;
		LoadFavourites(db);
		LoadPermaMirrors(db);
		LoadScoreGoals(db);
		LoadPlayLists(db);
		LoadPlayerScores(db);
	}
	catch (std::exception& e)
	{
		return ProfileLoadResult_FailedTampered;
	}
	delete db;
	return ProfileLoadResult_Success;
}

bool DBProfile::LoadGeneralData(SQLite::Database* db)
{
	SQLite::Statement   gDataQuery(*db, "SELECT * FROM generaldata");

	//Should only have one row so no executeStep loop
	if(!gDataQuery.executeStep())
		return false;
	loadingProfile->m_sDisplayName = static_cast<const char*>(gDataQuery.getColumn(1));
	loadingProfile->m_sCharacterID = static_cast<const char*>(gDataQuery.getColumn(2));
	loadingProfile->m_sLastUsedHighScoreName = static_cast<const char*>(gDataQuery.getColumn(3));
	loadingProfile->m_sGuid = static_cast<const char*>(gDataQuery.getColumn(4));
	loadingProfile->m_SortOrder = StringToSortOrder(static_cast<const char*>(gDataQuery.getColumn(5)));
	loadingProfile->m_LastDifficulty = static_cast<Difficulty>(static_cast<int>(gDataQuery.getColumn(6)));
	loadingProfile->m_LastStepsType = GAMEMAN->StringToStepsType(static_cast<const char*>(gDataQuery.getColumn(7)));

	const char* song = gDataQuery.getColumn(8);
	if(song != nullptr && song!="")
		loadingProfile->m_lastSong.LoadFromString(song);
	loadingProfile->m_iCurrentCombo = gDataQuery.getColumn(9);
	loadingProfile->m_iTotalSessions = gDataQuery.getColumn(10);
	loadingProfile->m_iTotalSessionSeconds = gDataQuery.getColumn(11);
	loadingProfile->m_iTotalGameplaySeconds = gDataQuery.getColumn(12);
	loadingProfile->m_LastPlayedDate.FromString(static_cast<const char*>(gDataQuery.getColumn(13)));
	loadingProfile->m_iTotalDancePoints = gDataQuery.getColumn(14);
	loadingProfile->m_iNumToasties = gDataQuery.getColumn(15);
	loadingProfile->m_iTotalTapsAndHolds = gDataQuery.getColumn(16);
	loadingProfile->m_iTotalJumps = gDataQuery.getColumn(17);
	loadingProfile->m_iTotalHolds = gDataQuery.getColumn(18);
	loadingProfile->m_iTotalRolls = gDataQuery.getColumn(19);
	loadingProfile->m_iTotalMines = gDataQuery.getColumn(20);
	loadingProfile->m_iTotalHands = gDataQuery.getColumn(21);
	loadingProfile->m_iTotalLifts = gDataQuery.getColumn(22);
	loadingProfile->m_fPlayerRating = static_cast<double>(gDataQuery.getColumn(23));

	SQLite::Statement   modifierQuery(*db, "SELECT * FROM defaultmodifiers");
	while(modifierQuery.executeStep())
	{
		const char* modifierName = modifierQuery.getColumn(1);
		const char* modifierValue = modifierQuery.getColumn(2);
		loadingProfile->m_sDefaultModifiers[modifierName] = modifierValue;
	}

	SQLite::Statement   skillsetsQuery(*db, "SELECT * FROM playerskillsets");
	while(skillsetsQuery.executeStep())
	{
		int skillsetNum = skillsetsQuery.getColumn(1);
		float skillsetValue = static_cast<double>(skillsetsQuery.getColumn(2));
		loadingProfile->m_fPlayerSkillsets[skillsetNum] = skillsetValue;	
	}

	SQLite::Statement   userTableQuery(*db, "SELECT * FROM usertable");

	Lua *L = LUA->Get();

	lua_newtable(L);

	//Only string values and no nested table support (TODO)
	while (userTableQuery.executeStep())
	{
		const char *key = userTableQuery.getColumn(0);
		const char *value = userTableQuery.getColumn(1);
		lua_pushstring(L, key);			// push key
		lua_pushstring(L, value);			// push value
		lua_settable(L, -3);
	}

	loadingProfile->m_UserTable.SetFromStack(L);
	LUA->Release(L);

	return true;
}
void DBProfile::LoadFavourites(SQLite::Database* db)
{
	SQLite::Statement   query(*db, "SELECT chartkeys.chartkey FROM favourites INNER JOIN chartkeys ON favourites.chartkeyid = chartkeys.id");
	while (query.executeStep())
	{
		const char* key = query.getColumn(0);
		//loadingProfile->FavoritedCharts.emplace(SONGMAN->ReconcileBustedKeys(key));
		loadingProfile->FavoritedCharts.emplace(key);
	}
	SONGMAN->SetFavoritedStatus(loadingProfile->FavoritedCharts);
	SONGMAN->MakePlaylistFromFavorites(loadingProfile->FavoritedCharts);
}


void DBProfile::LoadPlayLists(SQLite::Database* db) 
{
	//First load playlists
	SQLite::Statement   query(*db, "SELECT playlists.name, chartkeys.chartkey, "
		"charts.difficulty, songs.song, songs.pack, chartplaylists.rate "
		"FROM chartplaylists INNER JOIN playlists ON chartplaylists.playlistid = playlists.id "
		"INNER JOIN charts ON charts.id = chartplaylists.chartid "
		"INNER JOIN songs ON songs.id = charts.songid "
		"INNER JOIN chartkeys ON charts.chartkeyid = chartkeys.id "
		"ORDER BY playlists.name, chartkeys.chartkey, chartplaylists.rate");
	auto& pls = SONGMAN->allplaylists;

	Playlist *tmp;


	//Read one row
	if (query.executeStep()) {
		tmp = new Playlist;
		tmp->name = static_cast<const char*>(query.getColumn(0));
		
		//Load chart
		Chart ch;
		ch.key = static_cast<const char*>(query.getColumn(1));
		ch.lastdiff = static_cast<Difficulty>(static_cast<int>(query.getColumn(2)));
		ch.lastsong = static_cast<const char*>(query.getColumn(3));
		ch.lastpack = static_cast<const char*>(query.getColumn(4));
		ch.rate = static_cast<double>(query.getColumn(5));

		// check if this chart is loaded and overwrite any last-seen values with updated ones
		//ch.key = SONGMAN->ReconcileBustedKeys(ch.key);
		ch.FromKey(ch.key);

		//Add chart to playlist
		tmp->chartlist.emplace_back(ch);
	}
	else //Return if there are no palylists(There wont be course runs either)
		return;
	//Read the rest
	while (query.executeStep())
	{
		const char* curName = query.getColumn(0);
		const char* key = query.getColumn(1);

		if (curName != tmp->name) {
			//If the playlist changed add it and start a new one
			pls.emplace(tmp->name, *tmp);
			delete tmp;
			tmp = new Playlist;
			tmp->name = curName;
		}

		//Load the chart
		Chart ch;

		ch.lastdiff = static_cast<Difficulty>(static_cast<int>(query.getColumn(2)));
		ch.lastsong = static_cast<const char*>(query.getColumn(3));
		ch.lastpack = static_cast<const char*>(query.getColumn(4));
		ch.rate = static_cast<double>(query.getColumn(5));
		ch.key = key;

		// check if this chart is loaded and overwrite any last-seen values with updated ones
		//ch.key = SONGMAN->ReconcileBustedKeys(ch.key);
		ch.FromKey(ch.key);

		//Add chart to playlist
		tmp->chartlist.emplace_back(ch);

	}

	pls.emplace(tmp->name, *tmp);
	delete tmp;
	SONGMAN->activeplaylist = tmp->name;
	//Now read courseruns

	SQLite::Statement   courseRunsQuery(*db, "SELECT runs.scorekey, courseruns.id, playlists.name "
		"FROM runs INNER JOIN courseruns ON courseruns.id = runs.courserunid "
		"INNER JOIN playlists ON playlists.id = courseruns.playlistid "
		"ORDER BY runs.scorekey, courseruns.id, playlists.name");

	string lastPlayListName="";
	int lastCourseRunID;
	vector<string> tmpCourseRun;

	//Read one row
	if (courseRunsQuery.executeStep()) {

		const char* curScoreKey = query.getColumn(0);
		lastCourseRunID = query.getColumn(1);
		lastPlayListName = static_cast<const char*>(query.getColumn(2));

		tmpCourseRun.emplace_back(curScoreKey);

	}
	else
		return;

	//Read the rest
	while (query.executeStep())
	{
		const char* curScoreKey = query.getColumn(0);
		int curCourseRunID = query.getColumn(1);

		if (lastCourseRunID != curCourseRunID) {
			//If the courserun changed add it and start a new one
			pls[lastPlayListName].courseruns.emplace_back(tmpCourseRun);
			tmpCourseRun.clear();
			lastPlayListName = static_cast<const char*>(query.getColumn(2));
			lastCourseRunID = curCourseRunID;
		}

		tmpCourseRun.emplace_back(curScoreKey);
		
	}
	pls[lastPlayListName].courseruns.emplace_back(tmpCourseRun);
	tmpCourseRun.clear();
	return;
}

void DBProfile::LoadPlayerScores(SQLite::Database* db) 
{	
	SQLite::Statement   query(*db, "SELECT chartkeys.chartkey, "
		"songs.song, songs.pack, charts.difficulty, "
		"scoresatrates.rate, " // "scoresatrates.bestgrade, scoresatrates.pbkey, "
		"scores.scorekey, scores.calcversion, "
		"scores.grade, scores.wifescore, scores.ssrnormpercent, "
		"scores.judgescale, scores.nochordcohesion, scores.etternavalid, "
		"scores.surviveseconds, scores.maxcombo, scores.modifiers, scores.datetime, "
		"scores.hitmine, scores.avoidmine, scores.miss, scores.w5, scores.w4, scores.w3, "
		"scores.w2, scores.w1, scores.letgoholds, scores.heldholds, scores.missedholds, "
		"scores.overall, scores.stream, scores.jumpstream, "
		"scores.handstream, stamina, scores.jackspeed, "
		"scores.jackstamina, scores.technical, scores.brittle, scores.weak FROM scores "
		"INNER JOIN scoresatrates ON scoresatrates.id = scores.scoresatrateid "
		"INNER JOIN charts ON charts.id=scoresatrates.chartid "
		"INNER JOIN songs ON songs.id = charts.songid "
		"INNER JOIN chartkeys ON chartkeys.id=charts.chartkeyid "
		"ORDER BY chartkeys.id, charts.id, scoresatrates.id");

	unordered_map<string, ScoresForChart>& scores = *(SCOREMAN->GetProfileScores());

	string curCK = "";

	while (query.executeStep())
	{
		const string key = static_cast<const char*>(query.getColumn(0));
		if (key != curCK)
		{
			//Per Chart
			curCK = key;
			scores[key].ch.key = key;
			scores[key].ch.lastsong = static_cast<const char*>(query.getColumn(1));
			scores[key].ch.lastpack = static_cast<const char*>(query.getColumn(2));
			scores[key].ch.lastdiff = static_cast<Difficulty>(static_cast<int>(query.getColumn(3)));
		}

		int rate = query.getColumn(4);

		//Per Score
		string ScoreKey = query.getColumn(5);
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetSSRCalcVersion(query.getColumn(6));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetGrade(static_cast<Grade>(static_cast<int>(query.getColumn(7))));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetWifeScore(static_cast<double>(query.getColumn(8)));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetSSRNormPercent(static_cast<double>(query.getColumn(9)));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetMusicRate(scores[key].KeyToRate(rate));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetJudgeScale(static_cast<double>(query.getColumn(10)));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetChordCohesion(static_cast<int>(query.getColumn(11))!=0);
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetEtternaValid(static_cast<int>(query.getColumn(12))!=0);
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetChartKey(key);
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetScoreKey(ScoreKey);
		//TODO:surviveseconds
		//scores[key].ScoresByRate[rate].scores[ScoreKey].SetSurviveSeconds(query.getColumn(13));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetMaxCombo(query.getColumn(14));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetModifiers(query.getColumn(15));
		DateTime d;
		d.FromString(static_cast<const char*>(query.getColumn(16)));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetDateTime(d);
		
		
		int index = 17;

		FOREACH_ENUM(TapNoteScore, tns)
			if (tns != TNS_None && tns != TNS_CheckpointMiss && tns != TNS_CheckpointHit)
				scores[key].ScoresByRate[rate].scores[ScoreKey].SetTapNoteScore(tns, query.getColumn(index++));

		FOREACH_ENUM(HoldNoteScore, hns)
			if (hns != HNS_None)
				scores[key].ScoresByRate[rate].scores[ScoreKey].SetHoldNoteScore(hns, query.getColumn(index++));
		
		//int index = 28;
		if (scores[key].ScoresByRate[rate].scores[ScoreKey].GetWifeScore() > 0.f) {
			FOREACH_ENUM(Skillset, ss)
				scores[key].ScoresByRate[rate].scores[ScoreKey].SetSkillsetSSR(ss, static_cast<double>(query.getColumn(index++)));
		}
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetValidationKey(ValidationKey_Brittle, static_cast<const char*>(query.getColumn(index++)));
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetValidationKey(ValidationKey_Weak, static_cast<const char*>(query.getColumn(index++)));

		//TODO: validation keys


		if (scores[key].ScoresByRate[rate].scores[ScoreKey].GetScoreKey() == "")
			scores[key].ScoresByRate[rate].scores[ScoreKey].SetScoreKey("S" + BinaryToHex(CryptManager::GetSHA1ForString(scores[key].ScoresByRate[rate].scores[ScoreKey].GetDateTime().GetString())));

		// Validate input.
		scores[key].ScoresByRate[rate].scores[ScoreKey].SetGrade(clamp(scores[key].ScoresByRate[rate].scores[ScoreKey].GetGrade(), Grade_Tier01, Grade_Failed));


		// Set any pb
		if (scores[key].ScoresByRate[rate].PBptr == nullptr)
			scores[key].ScoresByRate[rate].PBptr = &scores[key].ScoresByRate[rate].scores.find(ScoreKey)->second;
		else {
			// update pb if a better score is found
			if (scores[key].ScoresByRate[rate].PBptr->GetWifeScore() < scores[key].ScoresByRate[rate].scores[ScoreKey].GetWifeScore())
				scores[key].ScoresByRate[rate].PBptr = &scores[key].ScoresByRate[rate].scores.find(ScoreKey)->second;
		};

		scores[key].ScoresByRate[rate].bestGrade = min(scores[key].ScoresByRate[rate].scores[ScoreKey].GetWifeGrade(), scores[key].ScoresByRate[rate].bestGrade);

		// Very awkward, need to figure this out better so there isn't unnecessary redundancy between loading and adding
		SCOREMAN->RegisterScore(&scores[key].ScoresByRate[rate].scores.find(ScoreKey)->second);
		SCOREMAN->AddToKeyedIndex(&scores[key].ScoresByRate[rate].scores.find(ScoreKey)->second);

		scores[key].bestGrade = min(scores[key].ScoresByRate[rate].bestGrade, scores[key].bestGrade);
	}
	/*
// Read scores from xml
void ScoresForChart::LoadFromNode(const XNode* node, const string& ck) {
	RString rs = "";
	int rate;

	if(node->GetName() == "Chart")
		ch.LoadFromNode(node);

	if (node->GetName() == "ChartScores") {
		node->GetAttrValue("Key", rs);
		ch.key = rs;
		node->GetAttrValue("Song", ch.lastsong);
		node->GetAttrValue("Pack", ch.lastpack);
	}

	FOREACH_CONST_Child(node, p) {
		ASSERT(p->GetName() == "ScoresAt");
		p->GetAttrValue("Rate", rs);
		rate = 10 * StringToInt(rs.substr(0, 1) + rs.substr(2, 4));
		ScoresByRate[rate].LoadFromNode(p, ck, KeyToRate(rate));
		bestGrade = min(ScoresByRate[rate].bestGrade, bestGrade);
	}
}
void ScoreManager::LoadFromNode(const XNode * node) {
	FOREACH_CONST_Child(node, p) {
		//ASSERT(p->GetName() == "Chart");
		RString tmp;
		p->GetAttrValue("Key", tmp);
		string doot = SONGMAN->ReconcileBustedKeys(tmp);
		const string ck = doot;
		pscores[ck].LoadFromNode(p, ck);
	}

void ScoresAtRate::LoadFromNode(const XNode* node, const string& ck, const float& rate) {
	RString sk;
	FOREACH_CONST_Child(node, p) {
		p->GetAttrValue("Key", sk);
		scores[sk].LoadFromEttNode(p);

		// Set any pb
		if(PBptr == nullptr)
			PBptr = &scores.find(sk)->second;
		else {
			// update pb if a better score is found
			if (PBptr->GetWifeScore() < scores[sk].GetWifeScore())
				PBptr = &scores.find(sk)->second;
		}

		// Fill in stuff for the highscores
		scores[sk].SetChartKey(ck);
		scores[sk].SetScoreKey(sk);
		scores[sk].SetMusicRate(rate);

		bestGrade = min(scores[sk].GetWifeGrade(), bestGrade);

		// Very awkward, need to figure this out better so there isn't unnecessary redundancy between loading and adding
		SCOREMAN->RegisterScore(&scores.find(sk)->second);
		SCOREMAN->AddToKeyedIndex(&scores.find(sk)->second);
	}
}
void HighScoreImpl::LoadFromEttNode(const XNode *pNode) {
	//ASSERT(pNode->GetName() == "Score");

	RString s;	
	pNode->GetChildValue("SSRCalcVersion", SSRCalcVersion);
	pNode->GetChildValue("Grade", s);
	grade = StringToGrade(s);
	pNode->GetChildValue("WifeScore", fWifeScore);
	pNode->GetChildValue("SSRNormPercent", fSSRNormPercent);
	pNode->GetChildValue("Rate", fMusicRate);
	pNode->GetChildValue("JudgeScale", fJudgeScale);
	pNode->GetChildValue("NoChordCohesion", bNoChordCohesion);
	pNode->GetChildValue("EtternaValid", bEtternaValid);
	pNode->GetChildValue("SurviveSeconds", fSurviveSeconds);
	pNode->GetChildValue("MaxCombo", iMaxCombo);
	pNode->GetChildValue("Modifiers", s); sModifiers = s;
	pNode->GetChildValue("DateTime", s); dateTime.FromString(s);
	pNode->GetChildValue("ScoreKey", s); ScoreKey = s;

	const XNode* pTapNoteScores = pNode->GetChild("TapNoteScores");
	if (pTapNoteScores)
		FOREACH_ENUM(TapNoteScore, tns)
		pTapNoteScores->GetChildValue(TapNoteScoreToString(tns), iTapNoteScores[tns]);

	const XNode* pHoldNoteScores = pNode->GetChild("HoldNoteScores");
	if (pHoldNoteScores)
		FOREACH_ENUM(HoldNoteScore, hns)
		pHoldNoteScores->GetChildValue(HoldNoteScoreToString(hns), iHoldNoteScores[hns]);

	if (fWifeScore > 0.f) {
		const XNode* pSkillsetSSRs = pNode->GetChild("SkillsetSSRs");
		if (pSkillsetSSRs)
			FOREACH_ENUM(Skillset, ss)
			pSkillsetSSRs->GetChildValue(SkillsetToString(ss), fSkillsetSSRs[ss]);
	}

	if (fWifeScore > 0.f) {
		const XNode* pValidationKeys = pNode->GetChild("ValidationKeys");
		if (pValidationKeys) {
			pValidationKeys->GetChildValue(ValidationKeyToString(ValidationKey_Brittle), s); ValidationKeys[ValidationKey_Brittle] = s;
			pValidationKeys->GetChildValue(ValidationKeyToString(ValidationKey_Weak), s); ValidationKeys[ValidationKey_Weak] = s;
		}
	}

	if (ScoreKey == "")
		ScoreKey = "S" + BinaryToHex(CryptManager::GetSHA1ForString(dateTime.GetString()));

	// Validate input.
	grade = clamp(grade, Grade_Tier01, Grade_Failed);
}

*/
}
void DBProfile::LoadPermaMirrors(SQLite::Database* db) 
{
	SQLite::Statement   query(*db, "SELECT chartkeys.chartkey FROM permamirrors INNER "
		"JOIN chartkeys ON permamirrors.chartkeyid = chartkeys.id");
	while (query.executeStep())
	{
		const char* key = query.getColumn(0);
		//loadingProfile->PermaMirrorCharts.emplace(SONGMAN->ReconcileBustedKeys(key));
		loadingProfile->PermaMirrorCharts.emplace(key);
	}
	SONGMAN->SetPermaMirroredStatus(loadingProfile->PermaMirrorCharts);
}
void DBProfile::LoadScoreGoals(SQLite::Database* db)
{
	SQLite::Statement   query(*db, "SELECT chartkeys.chartkey, scoregoals.rate, scoregoals.priority, scoregoals.percent, "
		"scoregoals.timeassigned, scoregoals.timeachieved, scoregoals.scorekey, "
		"scoregoals.comment FROM scoregoals INNER JOIN chartkeys ON "
		"scoregoals.chartkeyid = chartkeys.id ORDER BY chartkeys.chartkey ASC");
	//string lastKey="";
	RString ck;
	while (query.executeStep())
	{
		ck = static_cast<const char*>(query.getColumn(0));
		/* comment this for now (Also uncomment lastkey if uncommenting this)
		//Check if its a new key or the one we were already working on
		if (curKey != lastKey && curKey != ck) {
			lastKey = curKey;
			ck = SONGMAN->ReconcileBustedKeys(lastKey);
		}
		*/

		//Load the scoregoal
		ScoreGoal sg;
		sg.rate = static_cast<double>(query.getColumn(1));
		sg.priority = query.getColumn(2);
		sg.percent = static_cast<double>(query.getColumn(3));
		sg.timeassigned.FromString(static_cast<const char*>(query.getColumn(4)));
		sg.timeachieved.FromString(static_cast<const char*>(query.getColumn(5)));
		sg.scorekey = static_cast<const char*>(query.getColumn(6));
		sg.comment = static_cast<const char*>(query.getColumn(7));
		//Add it to the GoalsForAChart goalmap[chart]
		loadingProfile->goalmap[ck].Add(sg);
	}
	
	SONGMAN->SetHasGoal(loadingProfile->goalmap);
}
ProfileLoadResult DBProfile::SaveDBToDir(RString dir, const Profile* profile) const
{
	SQLite::Database *db;
	try {
		// Open a database file
		db = new SQLite::Database(FILEMAN->ResolvePath(dir) + PROFILE_DB, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
	}
	catch (std::exception& e)
	{
		return ProfileLoadResult_FailedNoProfile;
	}
	try {
		// Use the db
		// Begin transaction
		SQLite::Transaction transaction(*db);
		//We need to initialize these tables here
		db->exec("CREATE TABLE IF NOT EXISTS chartkeys (id INTEGER PRIMARY KEY, chartkey TEXT)");
		db->exec("DROP TABLE IF EXISTS skillsets");
		db->exec("CREATE TABLE skillsets (skillsetnum INTEGER PRIMARY KEY, skillset TEXT)");
		FOREACH_ENUM(Skillset, ss)
			db->exec("INSERT INTO skillsets VALUES (" + to_string(static_cast<int>(ss)) + ", \"" + SkillsetToString(ss) + "\")");
		SaveGeneralData(db, profile);
		SaveFavourites(db, profile);
		SavePermaMirrors(db, profile);
		SaveScoreGoals(db, profile);
		//Make sure playlists are loaded after playerscores
		SavePlayerScores(db, profile);
		SavePlayLists(db, profile);
		transaction.commit();
	}
	catch (std::exception& e)
	{
		return ProfileLoadResult_FailedTampered;
	}
	delete db;
	return ProfileLoadResult_Success;

}

void DBProfile::SaveFavourites(SQLite::Database* db, const Profile* profile) const
{;

	db->exec("DROP TABLE IF EXISTS favourites");
	db->exec("CREATE TABLE favourites (id INTEGER PRIMARY KEY, chartkeyid INTEGER, CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) REFERENCES chartkeys(id))");
	if (!profile->FavoritedCharts.empty()) {
		FOREACHS_CONST(string, profile->FavoritedCharts, ck) {
			SQLite::Statement insertFav(*db, "INSERT INTO favourites VALUES (NULL, ?)");
			insertFav.bind(1, FindOrCreateChartKey(db, *ck));
			insertFav.exec();
		}
	}

}


void DBProfile::SaveGeneralData(SQLite::Database* db, const Profile* profile) const
{

	db->exec("DROP TABLE IF EXISTS generaldata");
	db->exec("CREATE TABLE generaldata (id INTEGER PRIMARY KEY, "
		"displayname TEXT, characterid TEXT, guid TEXT, sortorder TEXT, "
		"lastdiff INTEGER, laststeps TEXT, lastsong TEXT, totalsessions INTEGER, totalsessionseconds INTEGER, "
		"totalgameplayseconds INTEGER, lastplayedmachineguid TEXT, lastplayeddate DATE, "
		"totaldancepoints INTEGER, numtoasties INTEGER, totaltapsandholds INTEGER, "
		"totaljumps INTEGER, totalholds INTEGER, totalrolls INTEGER, totalmines INTEGER, "
		"totalhands INTEGER, totallifts INTEGER, rating DOUBLE, totalsongsplayed INTEGER)");

	SQLite::Statement insertGData(*db, "INSERT INTO generaldata VALUES (NULL, ?,"
		"?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
	insertGData.bind(1, profile->GetDisplayNameOrHighScoreName());
	insertGData.bind(2, profile->m_sCharacterID);
	insertGData.bind(3, profile->m_sGuid);
	insertGData.bind(4, SortOrderToString(profile->m_SortOrder));
	insertGData.bind(5, profile->m_LastDifficulty);
	insertGData.bind(6, 
		((profile->m_LastStepsType != StepsType_Invalid && profile->m_LastStepsType < NUM_StepsType) ?
			GAMEMAN->GetStepsTypeInfo(profile->m_LastStepsType).szName : ""));
	insertGData.bind(7, profile->m_lastSong.ToString());
	insertGData.bind(8, profile->m_iTotalSessions);
	insertGData.bind(9, profile->m_iTotalSessionSeconds);
	insertGData.bind(10, profile->m_iTotalGameplaySeconds);
	insertGData.bind(11, profile->m_sLastPlayedMachineGuid);
	insertGData.bind(12, profile->m_LastPlayedDate.GetString());
	insertGData.bind(13, profile->m_iTotalDancePoints);
	insertGData.bind(14, profile->m_iNumToasties);
	insertGData.bind(15, profile->m_iTotalTapsAndHolds);
	insertGData.bind(16, profile->m_iTotalJumps);
	insertGData.bind(17, profile->m_iTotalHolds);
	insertGData.bind(18, profile->m_iTotalRolls);
	insertGData.bind(19, profile->m_iTotalMines);
	insertGData.bind(20, profile->m_iTotalHands);
	insertGData.bind(21, profile->m_iTotalLifts);
	insertGData.bind(22, profile->m_fPlayerRating);
	insertGData.bind(23, profile->m_iNumTotalSongsPlayed);
	insertGData.exec();
	db->exec("DROP TABLE IF EXISTS defaultmodifiers");
	db->exec("CREATE TABLE defaultmodifiers (id INTEGER PRIMARY KEY, "
		"name TEXT, value TEXT)");
	FOREACHM_CONST(RString, RString, profile->m_sDefaultModifiers, it)
		db->exec("INSERT INTO defaultmodifiers VALUES (NULL, \"" + it->first + "\", \"" + it->second + "\")");

	db->exec("DROP TABLE IF EXISTS playerskillsets");
	db->exec("CREATE TABLE playerskillsets (id INTEGER PRIMARY KEY, "
		"skillsetnum INTEGER, value DOUBLE, "
		"CONSTRAINT fk_skillsetnum FOREIGN KEY (skillsetnum) REFERENCES skillsets(skillsetnum))");
	
	FOREACH_ENUM(Skillset, ss)
		db->exec("INSERT INTO playerskillsets VALUES (NULL, " + to_string(ss) +
			", " + to_string(profile->m_fPlayerSkillsets[ss]) + ")");

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


void DBProfile::MoveBackupToDir(const RString &sFromDir, const RString &sToDir)
{
	if (FILEMAN->IsAFile(sFromDir + PROFILE_DB))
		FILEMAN->Move(sFromDir + PROFILE_DB, sToDir + PROFILE_DB);
}

void DBProfile::SavePermaMirrors(SQLite::Database* db, const Profile* profile) const
{

	db->exec("DROP TABLE IF EXISTS permamirrors");
	db->exec("CREATE TABLE permamirrors (id INTEGER PRIMARY KEY, chartkeyid INTEGER, CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) REFERENCES chartkeys(id))");

	if (!profile->PermaMirrorCharts.empty()) {
		FOREACHS_CONST(string, profile->PermaMirrorCharts, it) {
			int chID = FindOrCreateChartKey(db, *it);
			db->exec("INSERT INTO permamirrors VALUES (NULL, " + to_string(chID) + ")");
		}
	}
}


void DBProfile::SaveScoreGoals(SQLite::Database* db, const Profile* profile) const
{

	db->exec("DROP TABLE IF EXISTS scoregoals");
	db->exec("CREATE TABLE scoregoals (id INTEGER PRIMARY KEY, chartkeyid INTEGER, "
		"rate FLOAT, priority INTEGER, percent FLOAT, timeassigned DATE, timeachieved DATE, "
		"scorekey TEXT, comment TEXT, CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) REFERENCES chartkeys(id))");

	if (!profile->goalmap.empty()) {
		FOREACHUM_CONST(string, GoalsForChart, profile->goalmap, i) {
			const GoalsForChart& cg = i->second;
			if (cg.goals.empty())
				continue;
			int chID = FindOrCreateChartKey(db, cg.goals[0].chartkey);
			FOREACH_CONST(ScoreGoal, cg.goals, sg) {
				SQLite::Statement insertScoreGoal(*db, "INSERT INTO scoregoals VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?)");
				insertScoreGoal.bind(1, chID);
				insertScoreGoal.bind(2, sg->rate);
				insertScoreGoal.bind(3, sg->priority);
				insertScoreGoal.bind(4, sg->percent);
				insertScoreGoal.bind(5, sg->timeassigned.GetString());
				if (sg->achieved) {
					insertScoreGoal.bind(6, sg->timeachieved.GetString());
					insertScoreGoal.bind(7, sg->scorekey);
				}
				else {
					//bind null values
					insertScoreGoal.bind(6);
					insertScoreGoal.bind(7);
				}
				insertScoreGoal.bind(8, sg->comment);
				insertScoreGoal.exec();
			}
		}
	}

}

void DBProfile::SavePlayLists(SQLite::Database* db, const Profile* profile) const
{

		db->exec("DROP TABLE IF EXISTS playlists");
		db->exec("CREATE TABLE playlists (id INTEGER PRIMARY KEY, name TEXT)");

		db->exec("DROP TABLE IF EXISTS chartplaylists");
		db->exec("CREATE TABLE chartplaylists (id INTEGER PRIMARY KEY, chartid INTEGER, "
			"playlistid INTEGER, rate FLOAT, "
			"CONSTRAINT fk_chartid FOREIGN KEY (chartid) REFERENCES charts(id), "
			"CONSTRAINT fk_playlistid FOREIGN KEY (playlistid) REFERENCES playlists(id))");

		db->exec("DROP TABLE IF EXISTS courseruns");
		db->exec("CREATE TABLE courseruns (id INTEGER PRIMARY KEY, playlistid INTEGER, "
			"CONSTRAINT fk_playlistid FOREIGN KEY (playlistid) REFERENCES playlists(id))");

		db->exec("DROP TABLE IF EXISTS runs");
		db->exec("CREATE TABLE runs (id INTEGER PRIMARY KEY, scorekey TEXT, "
			"courserunid INTEGER, "
			"CONSTRAINT fk_courserunid FOREIGN KEY (courserunid) REFERENCES courseruns(id))");


		auto& pls = SONGMAN->allplaylists;
		if (!pls.empty()) {
			FOREACHM(string, Playlist, pls, pl)
			{
				if (pl->first != "" && pl->first != "Favorites")
				{
					SQLite::Statement insertPlaylist(*db, "INSERT INTO playlists VALUES (NULL, ?)");
					insertPlaylist.bind(1, (pl->second).name);
					insertPlaylist.exec();
					//db->exec("INSERT INTO playlists VALUES (NULL, \"" + (pl->second).name + "\")");
					int plID = sqlite3_last_insert_rowid(db->getHandle());
					FOREACH_CONST(Chart, (pl->second).chartlist, ch)
					{

						int chartKeyID = FindOrCreateChartKey(db, ch->key);
						int chartID;

						int songID = FindOrCreateSong(db, ch->lastpack, ch->lastsong);

						//Find or create chart now
						//Check if chart already exists
						SQLite::Statement   query(*db, "SELECT * FROM charts WHERE chartkeyid=? AND songid=? AND difficulty=?");
						query.bind(1, chartKeyID);
						query.bind(2, songID);
						query.bind(3, ch->lastdiff);
						//if not
						if (!query.executeStep())
						{
							//insert the chart
							

							SQLite::Statement insertChart(*db, "INSERT INTO charts VALUES (NULL, ?, ?, ?)");

							insertChart.bind(1, chartKeyID);
							insertChart.bind(2, songID);
							insertChart.bind(3, ch->lastdiff);
							insertChart.exec();
							chartID = sqlite3_last_insert_rowid(db->getHandle());
						}
						else {
							chartID = query.getColumn(0);
						}
						SQLite::Statement insertChartPlaylist(*db, "INSERT INTO chartplaylists VALUES (NULL, ?, ?, ?)");
						insertChartPlaylist.bind(1, chartID);
						insertChartPlaylist.bind(2, plID);
						insertChartPlaylist.bind(3, ch->rate);
						insertChartPlaylist.exec();
					}
				
					FOREACH_CONST(vector<string>, (pl->second).courseruns, run) {

						SQLite::Statement insertCourseRun(*db, "INSERT INTO courseruns VALUES (NULL, ?)");
						insertCourseRun.bind(1, plID);
						insertCourseRun.exec();
						int courseRunID = sqlite3_last_insert_rowid(db->getHandle());
						FOREACH_CONST(string, *run, scorekey) {
							SQLite::Statement insertRun(*db, "INSERT INTO runs VALUES (NULL, ?, ?)");
							insertRun.bind(1, *scorekey);
							insertRun.bind(2, courseRunID);
							insertRun.exec();

						}
					}
				}
			}
		}

}


void DBProfile::SavePlayerScores(SQLite::Database* db, const Profile* profile) const
{


	db->exec("DROP TABLE IF EXISTS scores");
	db->exec("CREATE TABLE scores (id INTEGER PRIMARY KEY, scoresatrateid INTEGER, "
		"scorekey TEXT, calcversion INT, grade INTEGER, wifescore FLOAT, "
		"ssrnormpercent FLOAT, judgescale FLOAT, nochordcohesion INTEGER, "
		"etternavalid INTEGER, surviveseconds FLOAT, maxcombo INTEGER, modifiers TEXT, datetime DATE, "
		"hitmine INTEGER, avoidmine INTEGER, miss INTEGER, w5 INTEGER, w4 INTEGER, "
		"w3 INTEGER, w2 INTEGER, w1 INTEGER, letgoholds INTEGER, heldholds INTEGER, "
		"missedholds INTEGER, overall FLOAT, stream FLOAT, jumpstream FLOAT, "
		"handstream FLOAT, stamina FLOAT, jackspeed FLOAT, "
		"jackstamina FLOAT, technical FLOAT, brittle TEXT, weak TEXT, "
		"CONSTRAINT fk_scoresatrateid FOREIGN KEY (scoresatrateid) REFERENCES scoresatrates(id))");


	//TODO: validation keys


	db->exec("DROP TABLE IF EXISTS scoresatrates");
	db->exec("CREATE TABLE scoresatrates (id INTEGER PRIMARY KEY, chartid INTEGER, "
		"rate INTEGER, bestgrade TEXT, pbkey TEXT, "
		"CONSTRAINT fk_chartid FOREIGN KEY (chartid) REFERENCES charts(id))");

	/*
	db->exec("DROP TABLE IF EXISTS charts");
	db->exec("CREATE TABLE charts (id INTEGER PRIMARY KEY, chartkeyid INTEGER, "
		"pack TEXT, song TEXT, difficulty INTEGER, "
		"CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) REFERENCES chartkeys(id))");*/
	db->exec("DROP TABLE IF EXISTS charts");
	db->exec("CREATE TABLE charts (id INTEGER PRIMARY KEY, chartkeyid INTEGER, "
		"songid INTEGER, difficulty INTEGER, "
		"CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) REFERENCES chartkeys(id), "
		"CONSTRAINT fk_songid FOREIGN KEY (songid) REFERENCES songs(id))");

	db->exec("DROP TABLE IF EXISTS songs");
	db->exec("CREATE TABLE songs (id INTEGER PRIMARY KEY, "
		"pack TEXT, song TEXT)");

	unordered_map<string, ScoresForChart> & pScores = *SCOREMAN->GetProfileScores();
	FOREACHUM_CONST(string, ScoresForChart, pScores, chartPair) {
		// First is ckey and two is ScoresForChart

		Chart ch = ((chartPair->second).ch);
		ch.FromKey(chartPair->first);

		//add chart ch
		int chartKeyID = FindOrCreateChartKey(db, ch.key);

		int songID = FindOrCreateSong(db, ch.lastpack, ch.lastsong);

		SQLite::Statement insertChart(*db, "INSERT INTO charts VALUES (NULL, ?, ?, ?)");
		
		insertChart.bind(1, chartKeyID);
		insertChart.bind(2, songID);
		insertChart.bind(3, ch.lastdiff);
		
		insertChart.exec();
		int chartID = sqlite3_last_insert_rowid(db->getHandle());

		//Add scores per rate
		FOREACHM_CONST(int, ScoresAtRate, chartPair->second.ScoresByRate, ratePair) {
			//first is rate int and second is ScoresAtRate
			int rate = ratePair->first;
			SQLite::Statement insertScoresAtRate(*db, "INSERT INTO scoresatrates VALUES (NULL, ?, ?, ?, ?)");
			insertScoresAtRate.bind(1, chartID);
			insertScoresAtRate.bind(2, rate );
			insertScoresAtRate.bind(3, GradeToString(ratePair->second.bestGrade));
			insertScoresAtRate.bind(4, ratePair->second.PBptr->GetScoreKey());
			insertScoresAtRate.exec();
			int scoresAtRateID = sqlite3_last_insert_rowid(db->getHandle());
			FOREACHUM_CONST(string, HighScore, ratePair->second.scores, i) {
				// prune out sufficiently low scores
				if (i->second.GetWifeScore() > SCOREMAN->minpercent) {
					const HighScore* hs = &(i->second);
					//Add scores
					SQLite::Statement insertScore(*db, "INSERT INTO scores VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, "
							"?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
					insertScore.bind(1, scoresAtRateID);
					insertScore.bind(2, (hs->GetScoreKey() == "" ? "S" +
						BinaryToHex(CryptManager::GetSHA1ForString(hs->GetDateTime().GetString())) :
						hs->GetScoreKey()));
					insertScore.bind(3, hs->GetSSRCalcVersion());
					insertScore.bind(4, hs->GetWifeGrade());
					insertScore.bind(5, hs->GetWifeScore());
					insertScore.bind(6, hs->GetSSRNormPercent());
					insertScore.bind(7, hs->GetJudgeScale());
					insertScore.bind(8, hs->GetChordCohesion());
					insertScore.bind(9, hs->GetEtternaValid());
					insertScore.bind(10, hs->GetSurviveSeconds());
					insertScore.bind(11, hs->GetMaxCombo());
					insertScore.bind(12, hs->GetModifiers());
					insertScore.bind(13, hs->GetDateTime().GetString());

					int index = 14;
					FOREACH_ENUM(TapNoteScore, tns)
						if (tns != TNS_None && tns != TNS_CheckpointMiss && tns != TNS_CheckpointHit)
							insertScore.bind(index++, hs->GetTapNoteScore(tns));

					FOREACH_ENUM(HoldNoteScore, hns)
						if (hns != HNS_None)
							insertScore.bind(index++, hs->GetHoldNoteScore(hns));

					if (hs->GetWifeScore() > 0.f && hs->GetWifeGrade() != Grade_Failed && hs->GetSkillsetSSR(Skill_Overall) > 0.f) 
						FOREACH_ENUM(Skillset, ss)
							insertScore.bind(index++, hs->GetSkillsetSSR(ss));
					else
						FOREACH_ENUM(Skillset, ss)
							insertScore.bind(index++, 0);

					insertScore.bind(index++, hs->GetValidationKey(ValidationKey_Brittle));
					insertScore.bind(index++, hs->GetValidationKey(ValidationKey_Weak));
					insertScore.exec();
					
					/*
					int scoreID = sqlite3_last_insert_rowid(db->getHandle());
					// dont bother writing skillset ssrs for non-applicable scores
					if (hs->GetWifeScore() > 0.f && hs->GetWifeGrade() != Grade_Failed && hs->GetSkillsetSSR(Skill_Overall) > 0.f) {;
						FOREACH_ENUM(Skillset, ss) {
							SQLite::Statement insertSkillsetSSR(*db, "INSERT INTO skillsetssrs VALUES (NULL, " + to_string(scoreID) + ", ?, ?)");
							insertSkillsetSSR.bind(1, static_cast<int>(ss));
							insertSkillsetSSR.bind(2, hs->GetSkillsetSSR(ss)); 
							insertSkillsetSSR.exec();
						}
					}
					*/
				}
			}
		}
	}
}


int DBProfile::GetChartKeyID(SQLite::Database* db, RString key) const
{
	SQLite::Statement   query(*db, "SELECT * FROM chartkeys WHERE chartkey=?");
	query.bind(1, key);
	if (!query.executeStep())
		return 0;
	return query.getColumn(0);
}

RString DBProfile::GetChartKeyByID(SQLite::Database* db, int id) const
{
	SQLite::Statement   query(*db, "SELECT * FROM chartkeys WHERE id=?");
	query.bind(1, id);
	if (!query.executeStep())
		return "";
	return static_cast<const char*>(query.getColumn(1));
}


int DBProfile::FindOrCreateChartKey(SQLite::Database* db, RString key) const
{
	int exists = GetChartKeyID(db, key);
	if (exists)
		return exists;
	db->exec("INSERT INTO chartkeys VALUES (NULL, \"" + key + "\")");
	return sqlite3_last_insert_rowid(db->getHandle());
}


int DBProfile::FindOrCreateSong(SQLite::Database* db, string pack, string song) const
{
	SQLite::Statement   query(*db, "SELECT songs.id FROM songs WHERE song=? AND pack =?");
	query.bind(1, song);
	query.bind(2, pack);
	if (!query.executeStep()) {
		SQLite::Statement insertSong(*db, "INSERT INTO songs VALUES (NULL, ?, ?)");

		insertSong.bind(1, pack);
		insertSong.bind(2, song);
		insertSong.exec();
		return sqlite3_last_insert_rowid(db->getHandle());
	}
	return query.getColumn(0);
}
