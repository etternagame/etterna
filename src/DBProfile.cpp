
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
}

bool DBProfile::LoadGeneralData(SQLite::Database* db)
{
	SQLite::Statement   gDataQuery(*db, "SELECT * FROM generaldata");

	//Should only have one row so no executeStep loop
	if(!gDataQuery.executeStep())
		return false;
	profile->m_sDisplayName = static_cast<const char*>(gDataQuery.getColumn(1));
	profile->m_sCharacterID = static_cast<const char*>(gDataQuery.getColumn(2));
	profile->m_sLastUsedHighScoreName = static_cast<const char*>(gDataQuery.getColumn(3));
	profile->m_sGuid = static_cast<const char*>(gDataQuery.getColumn(4));
	profile->m_SortOrder = StringToSortOrder(static_cast<const char*>(gDataQuery.getColumn(5)));
	profile->m_LastDifficulty = StringToDifficulty(static_cast<const char*>(gDataQuery.getColumn(6)));
	profile->m_LastStepsType = GAMEMAN->StringToStepsType(static_cast<const char*>(gDataQuery.getColumn(7)));

	const char* song = gDataQuery.getColumn(8);
	if(song != nullptr && song!="")
		profile->m_lastSong.LoadFromString(song);
	profile->m_iCurrentCombo = gDataQuery.getColumn(9);
	profile->m_iTotalSessions = gDataQuery.getColumn(10);
	profile->m_iTotalSessionSeconds = gDataQuery.getColumn(11);
	profile->m_iTotalGameplaySeconds = gDataQuery.getColumn(12);
	profile->m_LastPlayedDate.FromString(static_cast<const char*>(gDataQuery.getColumn(13)));
	profile->m_iTotalDancePoints = gDataQuery.getColumn(14);
	profile->m_iNumToasties = gDataQuery.getColumn(15);
	profile->m_iTotalTapsAndHolds = gDataQuery.getColumn(16);
	profile->m_iTotalJumps = gDataQuery.getColumn(17);
	profile->m_iTotalHolds = gDataQuery.getColumn(18);
	profile->m_iTotalRolls = gDataQuery.getColumn(19);
	profile->m_iTotalMines = gDataQuery.getColumn(20);
	profile->m_iTotalHands = gDataQuery.getColumn(21);
	profile->m_iTotalLifts = gDataQuery.getColumn(22);
	profile->m_fPlayerRating = static_cast<double>(gDataQuery.getColumn(23));

	SQLite::Statement   modifierQuery(*db, "SELECT * FROM defaultmodifiers");
	while(modifierQuery.executeStep())
	{
		const char* modifierName = modifierQuery.getColumn(1);
		const char* modifierValue = modifierQuery.getColumn(2);
		profile->m_sDefaultModifiers[modifierName] = modifierValue;
	}

	SQLite::Statement   skillsetsQuery(*db, "SELECT * FROM playerskillsets");
	while(modifierQuery.executeStep())
	{
		int skillsetNum = skillsetsQuery.getColumn(1);
		float skillsetValue = static_cast<double>(skillsetsQuery.getColumn(2));
		profile->m_fPlayerSkillsets[skillsetNum] = skillsetValue;	
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

	profile->m_UserTable.SetFromStack(L);
	LUA->Release(L);

	return true;
}
void DBProfile::LoadFavourites(SQLite::Database* db)
{
	SQLite::Statement   query(*db, "SELECT * FROM favourites");
	while (query.executeStep())
	{
		const char* key = query.getColumn(1);
		profile->FavoritedCharts.emplace(SONGMAN->ReconcileBustedKeys(key));
	}
	SONGMAN->SetFavoritedStatus(profile->FavoritedCharts);
	SONGMAN->MakePlaylistFromFavorites(profile->FavoritedCharts);
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
ProfileLoadResult DBProfile::SaveDBToDir(RString dir) const
{

	SQLite::Database *db;
	RString a;
	try {
		// Open a database file
		a = FILEMAN->ResolvePath(dir) + PROFILE_DB;
		db = new SQLite::Database(a, SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
	}
	catch (std::exception& e)
	{
		return ProfileLoadResult_FailedNoProfile;
	}
	try {
		// Use the db
		//We need to initialize this table here
		db->exec("CREATE TABLE IF NOT EXISTS charts (id INTEGER PRIMARY KEY, key TEXT)");
		SaveGeneralData(db);
		SaveFavourites(db);
		SavePermaMirrors(db);
		SaveScoreGoals(db);
		SavePlayLists(db);
		SavePlayerScores(db);
	}
	catch (std::exception& e)
	{
		return ProfileLoadResult_FailedTampered;
	}
	return ProfileLoadResult_Success;

}

void DBProfile::SaveFavourites(SQLite::Database* db) const
{
	// Begin transaction
	SQLite::Transaction transaction(*db);

	db->exec("DROP TABLE IF EXISTS favourites");
	db->exec("CREATE TABLE favourites (id INTEGER PRIMARY KEY, FOREIGN KEY(chartid) REFERENCES charts(id))");

	FOREACHS_CONST(string, profile->FavoritedCharts, ck) {
		int chID = AddOrCreateChart(db, *ck);
		int nb = db->exec("INSERT INTO favourites VALUES (NULL, " + std::to_string(chID) + ")");
	}

	// Commit transaction
	transaction.commit();
}


void DBProfile::SaveGeneralData(SQLite::Database* db) const
{
	// Begin transaction
	SQLite::Transaction transaction(*db);

	db->exec("DROP TABLE IF EXISTS generaldata");
	db->exec("CREATE TABLE generaldata (id INTEGER PRIMARY KEY, "
		"displayname TEXT, characterid TEXT, guid TEXT, sortorder TEXT, "
		"lastdiff TEXT, laststeps TEXT, lastsong TEXT, totalsessions INTEGER, totalsessionseconds INTEGER, "
		"totalgameplayseconds INTEGER, lastplayedmachineguid TEXT, lastplayeddate TEXT, "
		"totaldancepoints INTEGER, numtoasties INTEGER, totaltapsandholds INTEGER, "
		"totaljumps INTEGER, totalholds INTEGER, totalrolls INTEGER, totalmines INTEGER, "
		"totalhands INTEGER, totallifts INTEGER, rating DOUBLE, totalsongsplayed INTEGER)");

	db->exec("INSERT INTO generaldata VALUES (NULL, \"" +
	profile->GetDisplayNameOrHighScoreName() + "\", \"" +
	profile->m_sCharacterID + "\", \"" +
	profile->m_sGuid + "\", \"" +
	SortOrderToString(profile->m_SortOrder) + "\", \"" +
	DifficultyToString(profile->m_LastDifficulty) + "\", \"" +
	( (profile->m_LastStepsType != StepsType_Invalid && profile->m_LastStepsType < NUM_StepsType) ?
		GAMEMAN->GetStepsTypeInfo(profile->m_LastStepsType).szName : "" ) + 
	"\", \"" + profile->m_lastSong.ToString() + 
	"\", " + std::to_string(profile->m_iTotalSessions) +
	", " + std::to_string(profile->m_iTotalSessionSeconds) +
	", " + std::to_string(profile->m_iTotalGameplaySeconds) +
	", \"" + profile->m_sLastPlayedMachineGuid +
	"\", \"" + profile->m_LastPlayedDate.GetString() +
	"\", " + std::to_string(profile->m_iTotalDancePoints) +
	", " + std::to_string(profile->m_iNumToasties) +
	", " + std::to_string(profile->m_iTotalTapsAndHolds) +
	", " + std::to_string(profile->m_iTotalJumps) +
	", " + std::to_string(profile->m_iTotalHolds) +
	", " + std::to_string(profile->m_iTotalRolls) +
	", " + std::to_string(profile->m_iTotalMines) +
	", " + std::to_string(profile->m_iTotalHands) +
	", " + std::to_string(profile->m_iTotalLifts) +
	", " + std::to_string(profile->m_fPlayerRating) +
	", " + std::to_string(profile->m_iNumTotalSongsPlayed) +
	")");
	//*/
	db->exec("DROP TABLE IF EXISTS defaultmodifiers");
	db->exec("CREATE TABLE defaultmodifiers (id INTEGER PRIMARY KEY, "
		"num INTEGER, value TEXT)");
	FOREACHM_CONST(RString, RString, profile->m_sDefaultModifiers, it)
		db->exec("INSERT INTO defaultmodifiers VALUES (NULL, \"" + it->first + "\", \"" + it->second + "\")");

	db->exec("DROP TABLE IF EXISTS playerskillsets");
	db->exec("CREATE TABLE playerskillsets (id INTEGER PRIMARY KEY, "
		"num INTEGER, value DOUBLE)");
	FOREACH_ENUM(Skillset, ss)
		db->exec("INSERT INTO defaultmodifiers VALUES (NULL, \"" + std::to_string(ss) +
			"\", \"" + std::to_string(profile->m_fPlayerSkillsets[ss]) + "\")");

	db->exec("DROP TABLE IF EXISTS usertable");
	db->exec("CREATE TABLE usertable (id INTEGER PRIMARY KEY, "
		"key TEXT, value TEXT)");
	
	if (profile->m_UserTable.IsSet())
	{
		Lua *L = LUA->Get();
		profile->m_UserTable.PushSelf(L);
		//XNode* pUserTable = XmlFileUtil::XNodeFromTable(L);
		//TODO
		LUA->Release(L);
	}

	// Commit transaction
	transaction.commit();


}


void DBProfile::MoveBackupToDir(const RString &sFromDir, const RString &sToDir)
{
	if (FILEMAN->IsAFile(sFromDir + PROFILE_DB))
		FILEMAN->Move(sFromDir + PROFILE_DB, sToDir + PROFILE_DB);
}

void DBProfile::SavePermaMirrors(SQLite::Database* db) const 
{
	// Begin transaction
	SQLite::Transaction transaction(*db);

	db->exec("DROP TABLE IF EXISTS permamirrors");
	db->exec("CREATE TABLE permamirrors (id INTEGER PRIMARY KEY, FOREIGN KEY(chartid) REFERENCES charts(id))");

	FOREACHS_CONST(string, profile->PermaMirrorCharts, it) {
		int chID = AddOrCreateChart(db, *it);
		db->exec("INSERT INTO permamirrors VALUES (NULL, "+std::to_string(chID)+")");
	}
	/*
XNode* XMLProfile::SavePermaMirrorCreateNode() const {
	CHECKPOINT_M("Saving the permamirror node.");

	XNode* pmir = new XNode("PermaMirror");
	FOREACHS_CONST(string, profile->PermaMirrorCharts, it)
		pmir->AppendChild(*it);
	return pmir;
}*/
}


void DBProfile::SaveScoreGoals(SQLite::Database* db) const 
{
	// Begin transaction
	SQLite::Transaction transaction(*db);

	db->exec("DROP TABLE IF EXISTS scoregoals");
	db->exec("CREATE TABLE scoregoals (id INTEGER PRIMARY KEY, FOREIGN KEY(chartid) REFERENCES charts(id), "
		"rate FLOAT, priority INTEGER, percent FLOAT, timeassigned TEXT, timeachieved TEXT, scorekey TEXT, comment TEXT)");

	FOREACHUM_CONST(string, GoalsForChart, profile->goalmap, i) {
		const GoalsForChart& cg = i->second;
		if (cg.goals.empty())
			continue;
		int chID=AddOrCreateChart(db, cg.goals[0].chartkey);
		FOREACH_CONST(ScoreGoal, cg.goals, sg)
			db->exec("INSERT INTO scoregoals VALUES (NULL, "+std::to_string(chID)+", "+ 
				std::to_string(sg->rate) +", "+ std::to_string(sg->priority) +", "+ 
				std::to_string(sg->percent) +", \""+sg->timeassigned.GetString()+"\", \""+
				(sg->achieved ? (sg->timeachieved.GetString()+"\", \""+sg->scorekey) : "\", \"")+
				"\", \""+sg->comment+"\")");
	}
	/*
	XNode* ScoreGoal::CreateNode() const {
	XNode* pNode = new XNode("ScoreGoal");

	pNode->AppendChild("Rate", rate);
	pNode->AppendChild("Percent", percent);
	pNode->AppendChild("Priority", priority);
	pNode->AppendChild("Achieved", achieved);
	pNode->AppendChild("TimeAssigned", timeassigned.GetString());
	if (achieved) {
		pNode->AppendChild("TimeAchieved", timeachieved.GetString());
		pNode->AppendChild("ScoreKey", scorekey);
	}
		
	pNode->AppendChild("Comment", comment);

	return pNode;
}
XNode* XMLProfile::SaveScoreGoalsCreateNode() const {
	CHECKPOINT_M("Saving the scoregoals node.");

	XNode* goals = new XNode("ScoreGoals");
	FOREACHUM_CONST(string, GoalsForChart, profile->goalmap, i) {
		const GoalsForChart& cg = i->second;
		goals->AppendChild(cg.CreateNode());
	}
	return goals;
}

XNode* GoalsForChart::CreateNode() const {
XNode* cg = new XNode("GoalsForChart");

if (!goals.empty()) {
cg->AppendAttr("Key", goals[0].chartkey);
FOREACH_CONST(ScoreGoal, goals, sg)
cg->AppendChild(sg->CreateNode());
}
return cg;
}
*/
}
void DBProfile::SavePlayLists(SQLite::Database* db) const 
{
		// Begin transaction
		SQLite::Transaction transaction(*db);

		db->exec("DROP TABLE IF EXISTS playlists");
		db->exec("CREATE TABLE playlists (id INTEGER PRIMARY KEY, name TEXT)");
		db->exec("DROP TABLE IF EXISTS chartplaylists");
		db->exec("CREATE TABLE chartplaylists (id INTEGER PRIMARY KEY, FOREIGN KEY(chartid) REFERENCES charts(id), "
			"FOREIGN KEY(playlistid) REFERENCES playlists(id), lastpack TEXT, lastsong TEXT, laststeps TEXT)");

		auto& pls = SONGMAN->allplaylists;
		FOREACHM(string, Playlist, pls, pl)
		{
			if (pl->first != "" && pl->first != "Favorites")
			{
				db->exec("INSERT INTO playlists VALUES (NULL, \"" + (pl->second).name + "\")");
				int plID = sqlite3_last_insert_rowid(db->getHandle());
				FOREACH_CONST(Chart, (pl->second).chartlist, ch)
				{
					int chID = AddOrCreateChart(db, ch->key);
					db->exec("INSERT INTO chartplaylists VALUES (NULL, " + std::to_string(chID) + 
						", "+ std::to_string(plID) + ", \"" + ch->lastpack + "\")" + ", \"" + 
						ch->lastsong + "\")" + ", \"" + DifficultyToString(ch->lastdiff) + "\")");
				}
				//TODO : courseruns, cl, cr
			}
		}

		/*
		
XNode* Chart::CreateNode(bool includerate) const {
	XNode* ch = new XNode("Chart");
	ch->AppendAttr("Key", key);
	ch->AppendAttr("Pack", lastpack);
	ch->AppendAttr("Song", lastsong);
	ch->AppendAttr("Steps", DifficultyToString(lastdiff));

	if(includerate)
		ch->AppendAttr("Rate", ssprintf("%.3f",rate));
	return ch;
}
		XNode* Playlist::CreateNode() const {
	XNode* pl = new XNode("Playlist");
	pl->AppendAttr("Name", name);

	XNode* cl = new XNode("Chartlist");
	FOREACH_CONST(Chart, chartlist, ch)
		cl->AppendChild(ch->CreateNode(true));

	XNode* cr = new XNode("CourseRuns");
	FOREACH_CONST(vector<string>, courseruns, run) {
		XNode* r = new XNode("Run");
		FOREACH_CONST(string, *run, sk)
			r->AppendChild(*sk);
		cr->AppendChild(r);
	}

	if (!cl->ChildrenEmpty())
		pl->AppendChild(cl);

	if (!cr->ChildrenEmpty())
		pl->AppendChild(cr);
	
	return pl;
}
XNode* XMLProfile::SavePlaylistsCreateNode() const {
	CHECKPOINT_M("Saving the playlists node.");

	XNode* playlists = new XNode("Playlists");
	auto& pls = SONGMAN->allplaylists;
	FOREACHM(string, Playlist, pls, i)
		if (i->first != "" && i->first != "Favorites")
			playlists->AppendChild(i->second.CreateNode());
	return playlists;
}*/
}
void DBProfile::SavePlayerScores(SQLite::Database* db) const 
{
	// Begin transaction
	SQLite::Transaction transaction(*db);

	db->exec("DROP TABLE IF EXISTS playerscores");
	db->exec("CREATE TABLE playerscores (id INTEGER PRIMARY KEY)");

	db->exec("INSERT INTO playerscores VALUES (NULL");
	/*
XNode* SongID::CreateNode() const
{
	XNode* pNode = new XNode( "Song" );
	pNode->AppendAttr( "Dir", sDir );
	return pNode;
}
XNode* StepsID::CreateNode() const
{
	XNode* pNode = new XNode( "Steps" );

	pNode->AppendAttr( "StepsType", GAMEMAN->GetStepsTypeInfo(st).szName );
	pNode->AppendAttr( "Difficulty", DifficultyToString(dc) );
	pNode->AppendAttr(" ChartKey", ck);
	if( dc == Difficulty_Edit )
	{
		pNode->AppendAttr( "Description", sDescription );
		pNode->AppendAttr( "Hash", uHash );
	}

	return pNode;
}
XNode* HighScoreList::CreateNode() const
{
	XNode* pNode = new XNode( "HighScoreList" );

	pNode->AppendChild( "NumTimesPlayed", iNumTimesPlayed );
	pNode->AppendChild( "LastPlayed", dtLastPlayed.GetString() );
	if( HighGrade != Grade_NoData )
		pNode->AppendChild( "HighGrade", GradeToString(HighGrade) );

	for( unsigned i=0; i<vHighScores.size(); i++ )
	{
		const HighScore &hs = vHighScores[i];
		pNode->AppendChild( hs.CreateNode() );
	}

	return pNode;
}
XNode* HighScore::CreateNode() const
{
	return m_Impl->CreateNode();
}
XNode *HighScoreImpl::CreateNode() const
{
	XNode *pNode = new XNode( "HighScore" );

	// TRICKY:  Don't write "name to fill in" markers.
	pNode->AppendChild( "Name",				IsRankingToFillIn(sName) ? RString("") : sName );
	pNode->AppendChild( "HistoricChartKey", ChartKey);
	pNode->AppendChild( "ScoreKey",			ScoreKey);
	pNode->AppendChild( "SSRCalcVersion",	SSRCalcVersion);
	pNode->AppendChild( "Grade",			GradeToString(grade) );
	pNode->AppendChild( "Score",			iScore );
	pNode->AppendChild( "PercentDP",		fPercentDP );
	pNode->AppendChild( "WifeScore",		fWifeScore);
	pNode->AppendChild( "SSRNormPercent",	fSSRNormPercent);
	pNode->AppendChild( "Rate",				fMusicRate);
	pNode->AppendChild( "JudgeScale",		fJudgeScale);
	pNode->AppendChild( "NoChordCohesion",	bNoChordCohesion);
	pNode->AppendChild( "EtternaValid",		bEtternaValid);

	if (vOffsetVector.size() > 1) {
		pNode->AppendChild("Offsets", OffsetsToString(vOffsetVector));
		pNode->AppendChild("NoteRows", NoteRowsToString(vNoteRowVector));
	}
	
	pNode->AppendChild( "SurviveSeconds",	fSurviveSeconds );
	pNode->AppendChild( "MaxCombo",			iMaxCombo );
	pNode->AppendChild( "StageAward",		StageAwardToString(stageAward) );
	pNode->AppendChild( "PeakComboAward",	PeakComboAwardToString(peakComboAward) );
	pNode->AppendChild( "Modifiers",		sModifiers );
	pNode->AppendChild( "DateTime",			dateTime.GetString() );
	pNode->AppendChild( "PlayerGuid",		sPlayerGuid );
	pNode->AppendChild( "MachineGuid",		sMachineGuid );
	pNode->AppendChild( "ProductID",		iProductID );

	XNode* pTapNoteScores = pNode->AppendChild( "TapNoteScores" );
	FOREACH_ENUM( TapNoteScore, tns )
		if( tns != TNS_None )	// HACK: don't save meaningless "none" count
			pTapNoteScores->AppendChild( TapNoteScoreToString(tns), iTapNoteScores[tns] );

	XNode* pHoldNoteScores = pNode->AppendChild( "HoldNoteScores" );
	FOREACH_ENUM( HoldNoteScore, hns )
		if( hns != HNS_None )	// HACK: don't save meaningless "none" count
			pHoldNoteScores->AppendChild( HoldNoteScoreToString(hns), iHoldNoteScores[hns] );

	// dont bother writing skillset ssrs for non-applicable scores
	if (fWifeScore > 0.f) {
		XNode* pSkillsetSSRs = pNode->AppendChild("SkillsetSSRs");
		FOREACH_ENUM(Skillset, ss)
			pSkillsetSSRs->AppendChild(SkillsetToString(ss), fSkillsetSSRs[ss]);
	}

	pNode->AppendChild( radarValues.CreateNode() );
	pNode->AppendChild( "LifeRemainingSeconds",	fLifeRemainingSeconds );
	pNode->AppendChild( "Disqualified",		bDisqualified);
	return pNode;
}
XNode* XMLProfile::SaveSongScoresCreateNode() const
{
	CHECKPOINT_M("Getting the node to save song scores.");

	ASSERT(profile != NULL);

	XNode* pNode = new XNode("SongScores");

	FOREACHM_CONST(SongID, Profile::HighScoresForASong, profile->m_SongHighScores, i)
	{
		const SongID &songID = i->first;
		const Profile::HighScoresForASong &hsSong = i->second;

		// skip songs that have never been played
		if (profile->GetSongNumTimesPlayed(songID) == 0)
			continue;

		XNode* pSongNode = pNode->AppendChild(songID.CreateNode());

		int jCheck2 = hsSong.m_StepsHighScores.size();
		int jCheck1 = 0;
		FOREACHM_CONST(StepsID, Profile::HighScoresForASteps, hsSong.m_StepsHighScores, j)
		{
			jCheck1++;
			ASSERT(jCheck1 <= jCheck2);
			const StepsID &stepsID = j->first;
			const Profile::HighScoresForASteps &hsSteps = j->second;

			const HighScoreList &hsl = hsSteps.hsl;

			// skip steps that have never been played
			if (hsl.GetNumTimesPlayed() == 0)
				continue;

			XNode* pStepsNode = pSongNode->AppendChild(stepsID.CreateNode());

			pStepsNode->AppendChild(hsl.CreateNode());
		}
	}

	return pNode;
}*/
}

int DBProfile::GetChartID(SQLite::Database* db, RString key) const
{
	SQLite::Statement   query(*db, "SELECT * FROM charts WHERE key=\"" + key + "\"");
	if (!query.executeStep())
		return 0;
	return query.getColumn(0);
}

RString DBProfile::GetChartByID(SQLite::Database* db, int id) const
{
	SQLite::Statement   query(*db, "SELECT * FROM charts WHERE id=" + std::to_string(id));
	if (!query.executeStep())
		return "";
	return static_cast<const char*>(query.getColumn(1));
}


int DBProfile::AddOrCreateChart(SQLite::Database* db, RString key) const
{
	int exists = GetChartID(db, key);
	if (exists)
		return exists;
	db->exec("INSERT INTO charts VALUES (NULL, \"" + key + "\")");
	return sqlite3_last_insert_rowid(db->getHandle());;
}