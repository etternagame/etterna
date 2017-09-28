
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
		db = new SQLite::Database(PROFILEMAN->GetProfileDir(static_cast<ProfileSlot>(0)) + dir + PROFILE_DB, SQLite::OPEN_READWRITE);
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
	loadingProfile->m_LastDifficulty = StringToDifficulty(static_cast<const char*>(gDataQuery.getColumn(6)));
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
	while(modifierQuery.executeStep())
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
	SQLite::Statement   query(*db, "SELECT * FROM favourites");
	while (query.executeStep())
	{
		const char* key = query.getColumn(1);
		loadingProfile->FavoritedCharts.emplace(SONGMAN->ReconcileBustedKeys(key));
	}
	SONGMAN->SetFavoritedStatus(loadingProfile->FavoritedCharts);
	SONGMAN->MakePlaylistFromFavorites(loadingProfile->FavoritedCharts);
}


void DBProfile::LoadPlayLists(SQLite::Database* db) 
{
}
void DBProfile::LoadPlayerScores(SQLite::Database* db) 
{
}
void DBProfile::LoadPermaMirrors(SQLite::Database* db) 
{
}
void DBProfile::LoadScoreGoals(SQLite::Database* db)
{
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
		"lastdiff TEXT, laststeps TEXT, lastsong TEXT, totalsessions INTEGER, totalsessionseconds INTEGER, "
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
	insertGData.bind(5, DifficultyToString(profile->m_LastDifficulty));
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
				SQLite::Statement insertScoreGoal(*db, "INSERT INTO scoregoals VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?");
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
			"playlistid INTEGER, "
			"CONSTRAINT fk_chartid FOREIGN KEY (chartid) REFERENCES charts(id), "
			"CONSTRAINT fk_playlistid FOREIGN KEY (playlistid) REFERENCES playlists(id))");

		auto& pls = SONGMAN->allplaylists;
		if (!pls.empty()) {
			FOREACHM(string, Playlist, pls, pl)
			{
				if (pl->first != "" && pl->first != "Favorites")
				{
					db->exec("INSERT INTO playlists VALUES (NULL, \"" + (pl->second).name + "\")");
					int plID = sqlite3_last_insert_rowid(db->getHandle());
					FOREACH_CONST(Chart, (pl->second).chartlist, ch)
					{

						int chartKeyID = FindOrCreateChartKey(db, ch->key);
						int chartID;
						//Check if chart already exists
						SQLite::Statement   query(*db, "SELECT * FROM charts WHERE chartkeyid=? AND song=? AND pack= AND difficulty=?");
						query.bind(1, chartKeyID);
						query.bind(2, ch->lastsong);
						query.bind(3, ch->lastpack);
						query.bind(4, DifficultyToString(ch->lastdiff));
						//if not
						if (!query.executeStep())
						{
							//insert the chart
							SQLite::Statement insertChart(*db, "INSERT INTO charts VALUES (NULL, ?, ?, ?, ?)");

							insertChart.bind(1, chartKeyID);
							insertChart.bind(2, ch->lastpack);
							insertChart.bind(3, ch->lastsong);
							insertChart.bind(4, DifficultyToString(ch->lastdiff));
							insertChart.exec();
							chartID = sqlite3_last_insert_rowid(db->getHandle());
						}
						else {
							chartID = query.getColumn(0);
						}
						SQLite::Statement insertChartPlaylist(*db, "INSERT INTO chartplaylists VALUES (NULL, ?, ?)");
						insertChartPlaylist.bind(1, chartID);
						insertChartPlaylist.bind(1, plID);
					}
					//TODO : courseruns, cl, cr
				}
			}
		}

}


void DBProfile::SavePlayerScores(SQLite::Database* db, const Profile* profile) const
{


	db->exec("DROP TABLE IF EXISTS scores");
	db->exec("CREATE TABLE scores (id INTEGER PRIMARY KEY, scoresatrateid INTEGER, "
		"scorekey TEXT, calcversion INT, grade TEXT, wifescore FLOAT, ssrnormpercent FLOAT, judgescale FLOAT, "
		"nochordcohesion INTEGER, etternavalid INTEGER, surviveseconds FLOAT, maxcombo INTEGER, modifiers TEXT, datetime DATE, "
		"hitmine INTEGER, avoidmine INTEGER, miss INTEGER, w5 INTEGER, w4 INTEGER, "
		"w INTEGER, w2 INTEGER, w1 INTEGER, letgoholds INTEGER, heldholds INTEGER, "
		"missedholds INTEGER, CONSTRAINT fk_scoresatrateid FOREIGN KEY (scoresatrateid) REFERENCES scoresatrates(id))");

	db->exec("DROP TABLE IF EXISTS skillsetssrs");
	db->exec("CREATE TABLE skillsetssrs (id INTEGER PRIMARY KEY, scoreid INTEGER, "
		"skillsetnum INTEGER, ssr FLOAT,"
		"CONSTRAINT fk_scoreid FOREIGN KEY (scoreid) REFERENCES scores(id), "
		"CONSTRAINT fk_skillsetnum FOREIGN KEY (skillsetnum) REFERENCES skillsets(skillsetnum))");

	

	//TODO: validation keys

	db->exec("DROP TABLE IF EXISTS scoresatrates");
	db->exec("CREATE TABLE scoresatrates (id INTEGER PRIMARY KEY, chartid INTEGER, "
		"rate FLOAT, bestgrade TEXT, pbkey TEXT, "
		"CONSTRAINT fk_chartid FOREIGN KEY (chartid) REFERENCES charts(id))");


	db->exec("DROP TABLE IF EXISTS charts");
	db->exec("CREATE TABLE charts (id INTEGER PRIMARY KEY, chartkeyid INTEGER, "
		"pack TEXT, song TEXT, difficulty TEXT, "
		"CONSTRAINT fk_chartkeyid FOREIGN KEY (chartkeyid) REFERENCES chartkeys(id))");
	unordered_map<string, ScoresForChart> & pScores = *SCOREMAN->GetProfileScores();
	FOREACHUM_CONST(string, ScoresForChart, pScores, chartPair) {
		// First is ckey and two is ScoresForChart

		Chart ch = ((chartPair->second).ch);
		ch.FromKey(chartPair->first);

		//add chart ch
		//We dont AddOrCreate since nothing has been added to the charts table so far
		int chartKeyID = FindOrCreateChartKey(db, ch.key);
		SQLite::Statement insertChart(*db, "INSERT INTO charts VALUES (NULL, ?, ?, ?, ?)");
		
		insertChart.bind(1, chartKeyID);
		insertChart.bind(2, ch.lastpack);
		insertChart.bind(3, ch.lastsong);
		insertChart.bind(4, DifficultyToString(ch.lastdiff));
		insertChart.exec();
		int chartID = sqlite3_last_insert_rowid(db->getHandle());

		//Add scores per rate
		FOREACHM_CONST(int, ScoresAtRate, chartPair->second.ScoresByRate, ratePair) {
			//first is rate int and second is ScoresAtRate
			int rate = ratePair->first;
			SQLite::Statement insertScoresAtRate(*db, "INSERT INTO scoresatrates VALUES (NULL, ?, ?, ?, ?)");
			insertScoresAtRate.bind(1, chartID);
			insertScoresAtRate.bind(2, (static_cast<float>(rate) / 10000.f) );
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
							"?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
					insertScore.bind(1, scoresAtRateID);
					insertScore.bind(2, (hs->GetScoreKey() == "" ? "S" +
						BinaryToHex(CryptManager::GetSHA1ForString(hs->GetDateTime().GetString())) :
						hs->GetScoreKey()));
					insertScore.bind(3, hs->GetSSRCalcVersion());
					insertScore.bind(4, GradeToString(hs->GetWifeGrade()));
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
					insertScore.exec();
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

