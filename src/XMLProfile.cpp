
#include "global.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "Profile.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ProfileManager.h"
#include "NoteData.h"
#include "XMLProfile.h"
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

const RString STATS_XML = "Stats.xml";
const RString STATS_XML_GZ = "Stats.xml.gz";
const RString ETT_XML = "Etterna.xml";
const RString ETT_XML_GZ = "Etterna.xml.gz";
/** @brief The filename containing the signature for STATS_XML's signature. */
const RString DONT_SHARE_SIG = "DontShare.sig";
static Preference<bool> g_bProfileDataCompress("ProfileDataCompress", false);


// Loading and saving
#define WARN_PARSER	ShowWarningOrTrace( __FILE__, __LINE__, "Error parsing file.", true )
#define WARN_AND_RETURN { WARN_PARSER; return; }
#define WARN_AND_CONTINUE { WARN_PARSER; continue; }
#define WARN_AND_BREAK { WARN_PARSER; break; }
#define WARN_M(m)	ShowWarningOrTrace( __FILE__, __LINE__, RString("Error parsing file: ")+(m), true )
#define WARN_AND_RETURN_M(m) { WARN_M(m); return; }
#define WARN_AND_CONTINUE_M(m) { WARN_M(m); continue; }
#define WARN_AND_BREAK_M(m) { WARN_M(m); break; }


#define LOAD_NODE(X)	{ \
	const XNode* X = xml->GetChild(#X); \
	if( X==NULL ) LOG->Warn("Failed to read section " #X); \
	else Load##X##FromNode(X); }
ProfileLoadResult XMLProfile::LoadStatsFromDir(RString dir, bool require_signature)
{
	dir += PROFILEMAN->GetStatsPrefix();
	this->profiledir = dir;
	// Check for the existance of stats.xml
	RString fn = dir + STATS_XML;
	bool compressed = false;
	if (!IsAFile(fn))
	{
		// Check for the existance of stats.xml.gz
		fn = dir + STATS_XML_GZ;
		compressed = true;
		if (!IsAFile(fn))
		{
			return ProfileLoadResult_FailedNoProfile;
		}
	};

	int iError;
	unique_ptr<RageFileBasic> pFile(FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == NULL)
	{
		LOG->Trace("Error opening %s: %s", fn.c_str(), strerror(iError));
		return ProfileLoadResult_FailedTampered;
	}

	if (compressed)
	{
		RString sError;
		uint32_t iCRC32;
		RageFileObjInflate *pInflate = GunzipFile(pFile.release(), sError, &iCRC32);
		if (pInflate == NULL)
		{
			LOG->Trace("Error opening %s: %s", fn.c_str(), sError.c_str());
			return ProfileLoadResult_FailedTampered;
		}

		pFile.reset(pInflate);
	}

	if (require_signature)
	{
		RString sStatsXmlSigFile = fn + SIGNATURE_APPEND;
		RString sDontShareFile = dir + DONT_SHARE_SIG;

		LOG->Trace("Verifying don't share signature \"%s\" against \"%s\"", sDontShareFile.c_str(), sStatsXmlSigFile.c_str());
		// verify the stats.xml signature with the "don't share" file
		if (!CryptManager::VerifyFileWithFile(sStatsXmlSigFile, sDontShareFile))
		{
			LuaHelpers::ReportScriptErrorFmt("The don't share check for '%s' failed.  Data will be ignored.", sStatsXmlSigFile.c_str());
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace("Done.");

		// verify stats.xml
		LOG->Trace("Verifying stats.xml signature");
		if (!CryptManager::VerifyFileWithFile(fn, sStatsXmlSigFile))
		{
			LuaHelpers::ReportScriptErrorFmt("The signature check for '%s' failed.  Data will be ignored.", fn.c_str());
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace("Done.");
	}

	LOG->Trace("Loading %s", fn.c_str());
	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return ProfileLoadResult_FailedTampered;
	LOG->Trace("Done.");

	return LoadStatsXmlFromNode(&xml);
}


ProfileLoadResult XMLProfile::LoadEttFromDir(RString dir) {
	dir += PROFILEMAN->GetStatsPrefix();
	profiledir = dir;
	loadingProfile->IsEtternaProfile = true;
	RString fn = dir + ETT_XML;
	bool compressed = false;
	if (!IsAFile(fn)) {
		fn = dir + STATS_XML_GZ;
		compressed = true;
		if (!IsAFile(fn))
			return ProfileLoadResult_FailedNoProfile;
	}

	int iError;
	unique_ptr<RageFileBasic> pFile(FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == NULL) {
		LOG->Trace("Error opening %s: %s", fn.c_str(), strerror(iError));
		return ProfileLoadResult_FailedTampered;
	}

	if (compressed) {
		RString sError;
		uint32_t iCRC32;
		RageFileObjInflate *pInflate = GunzipFile(pFile.release(), sError, &iCRC32);
		if (pInflate == NULL) {
			LOG->Trace("Error opening %s: %s", fn.c_str(), sError.c_str());
			return ProfileLoadResult_FailedTampered;
		}
		pFile.reset(pInflate);
	}

	LOG->Trace("Loading %s", fn.c_str());
	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return ProfileLoadResult_FailedTampered;
	LOG->Trace("Done.");

	return LoadEttXmlFromNode(&xml);
}

bool XMLProfile::SaveStatsXmlToDir(RString sDir, bool bSignData, const Profile* profile)
{
	LOG->Trace("SaveStatsXmlToDir: %s", sDir.c_str());
	unique_ptr<XNode> xml(SaveStatsXmlCreateNode(profile));

	sDir += PROFILEMAN->GetStatsPrefix();
	// Save stats.xml
	RString fn = sDir + (g_bProfileDataCompress ? STATS_XML_GZ : STATS_XML);

	{
		RString sError;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE))
		{
			LuaHelpers::ReportScriptErrorFmt("Couldn't open %s for writing: %s", fn.c_str(), f.GetError().c_str());
			return false;
		}

		if (g_bProfileDataCompress)
		{
			RageFileObjGzip gzip(&f);
			gzip.Start();
			if (!XmlFileUtil::SaveToFile(xml.get(), gzip, "", false))
				return false;

			if (gzip.Finish() == -1)
				return false;

			/* After successfully saving STATS_XML_GZ, remove any stray STATS_XML. */
			if (FILEMAN->IsAFile(sDir + STATS_XML))
				FILEMAN->Remove(sDir + STATS_XML);
		}
		else
		{
			if (!XmlFileUtil::SaveToFile(xml.get(), f, "", false))
				return false;

			/* After successfully saving STATS_XML, remove any stray STATS_XML_GZ. */
			if (FILEMAN->IsAFile(sDir + STATS_XML_GZ))
				FILEMAN->Remove(sDir + STATS_XML_GZ);
		}
	}

	if (bSignData)
	{
		RString sStatsXmlSigFile = fn + SIGNATURE_APPEND;
		CryptManager::SignFileToFile(fn, sStatsXmlSigFile);

		// Save the "don't share" file
		RString sDontShareFile = sDir + DONT_SHARE_SIG;
		CryptManager::SignFileToFile(sStatsXmlSigFile, sDontShareFile);
	}

	return true;
}

bool XMLProfile::SaveEttXmlToDir(RString sDir, const Profile* profile) const {
	LOG->Trace("Saving Etterna Profile to: %s", sDir.c_str());
	unique_ptr<XNode> xml(SaveEttXmlCreateNode(profile));
	sDir += PROFILEMAN->GetStatsPrefix();
	// Save Etterna.xml
	RString fn = sDir + ETT_XML;
	{
		RString sError;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE))
		{
			LuaHelpers::ReportScriptErrorFmt("Couldn't open %s for writing: %s", fn.c_str(), f.GetError().c_str());
			return false;
		}

		if (g_bProfileDataCompress)
		{
			RageFileObjGzip gzip(&f);
			gzip.Start();
			if (!XmlFileUtil::SaveToFile(xml.get(), gzip, "", false))
				return false;

			if (gzip.Finish() == -1)
				return false;

			/* After successfully saving STATS_XML_GZ, remove any stray STATS_XML. */
			if (FILEMAN->IsAFile(sDir + STATS_XML))
				FILEMAN->Remove(sDir + STATS_XML);
		}
		else
		{
			if (!XmlFileUtil::SaveToFile(xml.get(), f, "", false))
				return false;

			/* After successfully saving STATS_XML, remove any stray STATS_XML_GZ. */
			if (FILEMAN->IsAFile(sDir + STATS_XML_GZ))
				FILEMAN->Remove(sDir + STATS_XML_GZ);
		}
	}

	return true;
}

XNode* XMLProfile::SaveGeneralDataCreateNode(const Profile* profile) const
{
	XNode* pGeneralDataNode = new XNode("GeneralData");

	// TRICKY: These are write-only elements that are normally never read again.
	// This data is required by other apps (like internet ranking), but is 
	// redundant to the game app.
	pGeneralDataNode->AppendChild("DisplayName", profile->GetDisplayNameOrHighScoreName());
	pGeneralDataNode->AppendChild("CharacterID", profile->m_sCharacterID);
	pGeneralDataNode->AppendChild("LastUsedHighScoreName", profile->m_sLastUsedHighScoreName);
	pGeneralDataNode->AppendChild("Guid", profile->m_sGuid);
	pGeneralDataNode->AppendChild("SortOrder", SortOrderToString(profile->m_SortOrder));
	pGeneralDataNode->AppendChild("LastDifficulty", DifficultyToString(Difficulty_Invalid));
	if (profile->m_LastStepsType != StepsType_Invalid)
		pGeneralDataNode->AppendChild("LastStepsType", GAMEMAN->GetStepsTypeInfo(profile->m_LastStepsType).szName);
	pGeneralDataNode->AppendChild(profile->m_lastSong.CreateNode());
	pGeneralDataNode->AppendChild("CurrentCombo", profile->m_iCurrentCombo);
	pGeneralDataNode->AppendChild("TotalSessions", profile->m_iTotalSessions);
	pGeneralDataNode->AppendChild("TotalSessionSeconds", profile->m_iTotalSessionSeconds);
	pGeneralDataNode->AppendChild("TotalGameplaySeconds", profile->m_iTotalGameplaySeconds);
	pGeneralDataNode->AppendChild("LastPlayedMachineGuid", profile->m_sLastPlayedMachineGuid);
	pGeneralDataNode->AppendChild("LastPlayedDate", profile->m_LastPlayedDate.GetString());
	pGeneralDataNode->AppendChild("TotalDancePoints", profile->m_iTotalDancePoints);
	pGeneralDataNode->AppendChild("NumExtraStagesPassed", profile->m_iNumExtraStagesPassed);
	pGeneralDataNode->AppendChild("NumExtraStagesFailed", profile->m_iNumExtraStagesFailed);
	pGeneralDataNode->AppendChild("NumToasties", profile->m_iNumToasties);
	pGeneralDataNode->AppendChild("TotalTapsAndHolds", profile->m_iTotalTapsAndHolds);
	pGeneralDataNode->AppendChild("TotalJumps", profile->m_iTotalJumps);
	pGeneralDataNode->AppendChild("TotalHolds", profile->m_iTotalHolds);
	pGeneralDataNode->AppendChild("TotalRolls", profile->m_iTotalRolls);
	pGeneralDataNode->AppendChild("TotalMines", profile->m_iTotalMines);
	pGeneralDataNode->AppendChild("TotalHands", profile->m_iTotalHands);
	pGeneralDataNode->AppendChild("TotalLifts", profile->m_iTotalLifts);
	pGeneralDataNode->AppendChild("PlayerRating", profile->m_fPlayerRating);

	// Keep declared variables in a very local scope so they aren't 
	// accidentally used where they're not intended.  There's a lot of
	// copying and pasting in this code.

	{
		XNode* pDefaultModifiers = pGeneralDataNode->AppendChild("DefaultModifiers");
		FOREACHM_CONST(RString, RString, profile->m_sDefaultModifiers, it)
			pDefaultModifiers->AppendChild(it->first, it->second);
	}

	{
		XNode* pFavorites = pGeneralDataNode->AppendChild("Favorites");
		FOREACHS_CONST(string, profile->FavoritedCharts, it)
			pFavorites->AppendChild(*it);
	}

	{
		XNode* pPlayerSkillsets = pGeneralDataNode->AppendChild("PlayerSkillsets");
		FOREACH_ENUM(Skillset, ss)
			pPlayerSkillsets->AppendChild(SkillsetToString(ss), profile->m_fPlayerSkillsets[ss]);
	}

	{
		XNode* pNumSongsPlayedByPlayMode = pGeneralDataNode->AppendChild("NumSongsPlayedByPlayMode");
		FOREACH_ENUM(PlayMode, pm)
		{
			// Don't save unplayed PlayModes.
			if (!profile->m_iNumSongsPlayedByPlayMode[pm])
				continue;
			pNumSongsPlayedByPlayMode->AppendChild(PlayModeToString(pm), profile->m_iNumSongsPlayedByPlayMode[pm]);
		}
	}

	{
		XNode* pNumSongsPlayedByStyle = pGeneralDataNode->AppendChild("NumSongsPlayedByStyle");
		FOREACHM_CONST(StyleID, int, profile->m_iNumSongsPlayedByStyle, iter)
		{
			const StyleID &s = iter->first;
			int iNumPlays = iter->second;

			XNode *pStyleNode = s.CreateNode();
			pStyleNode->AppendAttr(XNode::TEXT_ATTRIBUTE, iNumPlays);

			pNumSongsPlayedByStyle->AppendChild(pStyleNode);
		}
	}

	{
		XNode* pNumSongsPlayedByDifficulty = pGeneralDataNode->AppendChild("NumSongsPlayedByDifficulty");
		FOREACH_ENUM(Difficulty, dc)
		{
			if (!profile->m_iNumSongsPlayedByDifficulty[dc])
				continue;
			pNumSongsPlayedByDifficulty->AppendChild(DifficultyToString(dc), profile->m_iNumSongsPlayedByDifficulty[dc]);
		}
	}

	{
		XNode* pNumSongsPlayedByMeter = pGeneralDataNode->AppendChild("NumSongsPlayedByMeter");
		for (int i = 0; i<MAX_METER + 1; i++)
		{
			if (!profile->m_iNumSongsPlayedByMeter[i])
				continue;
			pNumSongsPlayedByMeter->AppendChild(ssprintf("Meter%d", i), profile->m_iNumSongsPlayedByMeter[i]);
		}
	}

	pGeneralDataNode->AppendChild("NumTotalSongsPlayed", profile->m_iNumTotalSongsPlayed);

	{
		XNode* pNumStagesPassedByPlayMode = pGeneralDataNode->AppendChild("NumStagesPassedByPlayMode");
		FOREACH_ENUM(PlayMode, pm)
		{
			// Don't save unplayed PlayModes.
			if (!profile->m_iNumStagesPassedByPlayMode[pm])
				continue;
			pNumStagesPassedByPlayMode->AppendChild(PlayModeToString(pm), profile->m_iNumStagesPassedByPlayMode[pm]);
		}
	}

	{
		XNode* pNumStagesPassedByGrade = pGeneralDataNode->AppendChild("NumStagesPassedByGrade");
		FOREACH_ENUM(Grade, g)
		{
			if (!profile->m_iNumStagesPassedByGrade[g])
				continue;
			pNumStagesPassedByGrade->AppendChild(GradeToString(g), profile->m_iNumStagesPassedByGrade[g]);
		}
	}

	// Load Lua UserTable from profile
	if (profile->m_UserTable.IsSet())
	{
		Lua *L = LUA->Get();
		profile->m_UserTable.PushSelf(L);
		XNode* pUserTable = XmlFileUtil::XNodeFromTable(L);
		LUA->Release(L);

		// XXX: XNodeFromTable returns a root node with the name "Layer".
		pUserTable->m_sName = "UserTable";
		pGeneralDataNode->AppendChild(pUserTable);
	}

	return pGeneralDataNode;
}

XNode* XMLProfile::SaveFavoritesCreateNode(const Profile* profile) const {
	CHECKPOINT_M("Saving the favorites node.");

	XNode* favs = new XNode("Favorites");
	FOREACHS_CONST(string, profile->FavoritedCharts, it)
		favs->AppendChild(*it);
	return favs;
}

XNode* XMLProfile::SavePermaMirrorCreateNode(const Profile* profile) const {
	CHECKPOINT_M("Saving the permamirror node.");

	XNode* pmir = new XNode("PermaMirror");
	FOREACHS_CONST(string, profile->PermaMirrorCharts, it)
		pmir->AppendChild(*it);
	return pmir;
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

XNode* XMLProfile::SaveScoreGoalsCreateNode(const Profile* profile) const {
	CHECKPOINT_M("Saving the scoregoals node.");

	XNode* goals = new XNode("ScoreGoals");
	FOREACHUM_CONST(string, GoalsForChart, profile->goalmap, i) {
		const GoalsForChart& cg = i->second;
		goals->AppendChild(cg.CreateNode());
	}
	return goals;
}

XNode* XMLProfile::SavePlaylistsCreateNode(const Profile* profile) const {
	CHECKPOINT_M("Saving the playlists node.");

	XNode* playlists = new XNode("Playlists");
	auto& pls = SONGMAN->allplaylists;
	FOREACHM(string, Playlist, pls, i)
		if (i->first != "" && i->first != "Favorites")
			playlists->AppendChild(i->second.CreateNode());
	return playlists;
}

void XMLProfile::LoadFavoritesFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the favorites node.");

	FOREACH_CONST_Child(pNode, ck)
		loadingProfile->FavoritedCharts.emplace(SONGMAN->ReconcileBustedKeys(ck->GetName()));

	SONGMAN->SetFavoritedStatus(loadingProfile->FavoritedCharts);
	SONGMAN->MakePlaylistFromFavorites(loadingProfile->FavoritedCharts);
}

void XMLProfile::LoadPermaMirrorFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the permamirror node.");

	FOREACH_CONST_Child(pNode, ck)
		loadingProfile->PermaMirrorCharts.emplace(SONGMAN->ReconcileBustedKeys(ck->GetName()));

	SONGMAN->SetPermaMirroredStatus(loadingProfile->PermaMirrorCharts);
}

void GoalsForChart::LoadFromNode(const XNode *pNode) {
	FOREACH_CONST_Child(pNode, sg) {
		ScoreGoal doot;
		doot.LoadFromNode(sg);
		Add(doot);
	}
}

void XMLProfile::LoadScoreGoalsFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the scoregoals node.");

	RString ck;
	FOREACH_CONST_Child(pNode, chgoals) {
		chgoals->GetAttrValue("Key", ck);
		ck = SONGMAN->ReconcileBustedKeys(ck);
		loadingProfile->goalmap[ck].LoadFromNode(chgoals);
	}
	SONGMAN->SetHasGoal(loadingProfile->goalmap);
}

void XMLProfile::LoadPlaylistsFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the playlists node.");

	auto& pls = SONGMAN->allplaylists;
	FOREACH_CONST_Child(pNode, pl) {
		Playlist tmp;
		tmp.LoadFromNode(pl);
		pls.emplace(tmp.name, tmp);
		SONGMAN->activeplaylist = tmp.name;
	}
}


XNode* XMLProfile::SaveEttGeneralDataCreateNode(const Profile* profile) const {
	CHECKPOINT_M("Saving the general node.");

	XNode* pGeneralDataNode = new XNode("GeneralData");

	// TRICKY: These are write-only elements that are normally never read again.
	// This data is required by other apps (like internet ranking), but is 
	// redundant to the game app.
	pGeneralDataNode->AppendChild("DisplayName", profile->GetDisplayNameOrHighScoreName());
	pGeneralDataNode->AppendChild("CharacterID", profile->m_sCharacterID);
	pGeneralDataNode->AppendChild("Guid", profile->m_sGuid);
	pGeneralDataNode->AppendChild("SortOrder", SortOrderToString(profile->m_SortOrder));
	pGeneralDataNode->AppendChild("LastDifficulty", DifficultyToString(profile->m_LastDifficulty));
	if (profile->m_LastStepsType != StepsType_Invalid)
		pGeneralDataNode->AppendChild("LastStepsType", GAMEMAN->GetStepsTypeInfo(profile->m_LastStepsType).szName);
	pGeneralDataNode->AppendChild(profile->m_lastSong.CreateNode());
	pGeneralDataNode->AppendChild("TotalSessions", profile->m_iTotalSessions);
	pGeneralDataNode->AppendChild("TotalSessionSeconds", profile->m_iTotalSessionSeconds);
	pGeneralDataNode->AppendChild("TotalGameplaySeconds", profile->m_iTotalGameplaySeconds);
	pGeneralDataNode->AppendChild("LastPlayedMachineGuid", profile->m_sLastPlayedMachineGuid);
	pGeneralDataNode->AppendChild("LastPlayedDate", profile->m_LastPlayedDate.GetString());
	pGeneralDataNode->AppendChild("TotalDancePoints", profile->m_iTotalDancePoints);
	pGeneralDataNode->AppendChild("NumToasties", profile->m_iNumToasties);
	pGeneralDataNode->AppendChild("TotalTapsAndHolds", profile->m_iTotalTapsAndHolds);
	pGeneralDataNode->AppendChild("TotalJumps", profile->m_iTotalJumps);
	pGeneralDataNode->AppendChild("TotalHolds", profile->m_iTotalHolds);
	pGeneralDataNode->AppendChild("TotalRolls", profile->m_iTotalRolls);
	pGeneralDataNode->AppendChild("TotalMines", profile->m_iTotalMines);
	pGeneralDataNode->AppendChild("TotalHands", profile->m_iTotalHands);
	pGeneralDataNode->AppendChild("TotalLifts", profile->m_iTotalLifts);
	pGeneralDataNode->AppendChild("PlayerRating", profile->m_fPlayerRating);


	// Keep declared variables in a very local scope so they aren't 
	// accidentally used where they're not intended.  There's a lot of
	// copying and pasting in this code.

	{
		XNode* pDefaultModifiers = pGeneralDataNode->AppendChild("DefaultModifiers");
		FOREACHM_CONST(RString, RString, profile->m_sDefaultModifiers, it)
			pDefaultModifiers->AppendChild(it->first, it->second);
	}

	{
		XNode* pPlayerSkillsets = pGeneralDataNode->AppendChild("PlayerSkillsets");
		FOREACH_ENUM(Skillset, ss)
			pPlayerSkillsets->AppendChild(SkillsetToString(ss), profile->m_fPlayerSkillsets[ss]);
	}

	pGeneralDataNode->AppendChild("NumTotalSongsPlayed", profile->m_iNumTotalSongsPlayed);

	{
		XNode* pNumStagesPassedByPlayMode = pGeneralDataNode->AppendChild("NumStagesPassedByPlayMode");
		FOREACH_ENUM(PlayMode, pm)
		{
			// Don't save unplayed PlayModes.
			if (!profile->m_iNumStagesPassedByPlayMode[pm])
				continue;
			pNumStagesPassedByPlayMode->AppendChild(PlayModeToString(pm), profile->m_iNumStagesPassedByPlayMode[pm]);
		}
	}

	// Load Lua UserTable from profile
	if (profile->m_UserTable.IsSet())
	{
		Lua *L = LUA->Get();
		profile->m_UserTable.PushSelf(L);
		XNode* pUserTable = XmlFileUtil::XNodeFromTable(L);
		LUA->Release(L);

		// XXX: XNodeFromTable returns a root node with the name "Layer".
		pUserTable->m_sName = "UserTable";
		pGeneralDataNode->AppendChild(pUserTable);
	}

	return pGeneralDataNode;
}

void XMLProfile::LoadStatsXmlForConversion() {
	string dir = profiledir;
	RString fn = dir + STATS_XML;
	bool compressed = false;
	if (!IsAFile(fn)) {
		fn = dir + STATS_XML_GZ;
		compressed = true;
		if (!IsAFile(fn)) {
			return;
		}
	}

	int iError;
	unique_ptr<RageFileBasic> pFile(FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == NULL)
		return;

	if (compressed) {
		RString sError;
		uint32_t iCRC32;
		RageFileObjInflate *pInflate = GunzipFile(pFile.release(), sError, &iCRC32);
		if (pInflate == NULL)
			return;

		pFile.reset(pInflate);
	}

	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return;

	XNode* scores = xml.GetChild("SongScores");
	LoadSongScoresFromNode(scores);
}


void XMLProfile::MoveBackupToDir(const RString &sFromDir, const RString &sToDir)
{
	if (FILEMAN->IsAFile(sFromDir + STATS_XML) &&
		FILEMAN->IsAFile(sFromDir + STATS_XML + SIGNATURE_APPEND))
	{
		FILEMAN->Move(sFromDir + STATS_XML, sToDir + STATS_XML);
		FILEMAN->Move(sFromDir + STATS_XML + SIGNATURE_APPEND, sToDir + STATS_XML + SIGNATURE_APPEND);
	}
	else if (FILEMAN->IsAFile(sFromDir + STATS_XML_GZ) &&
		FILEMAN->IsAFile(sFromDir + STATS_XML_GZ + SIGNATURE_APPEND))
	{
		FILEMAN->Move(sFromDir + STATS_XML_GZ, sToDir + STATS_XML);
		FILEMAN->Move(sFromDir + STATS_XML_GZ + SIGNATURE_APPEND, sToDir + STATS_XML + SIGNATURE_APPEND);
	}
}


void XMLProfile::LoadGeneralDataFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "GeneralData");

	RString s;
	const XNode* pTemp;

	pNode->GetChildValue("DisplayName", loadingProfile->m_sDisplayName);
	pNode->GetChildValue("CharacterID", loadingProfile->m_sCharacterID);
	pNode->GetChildValue("LastUsedHighScoreName", loadingProfile->m_sLastUsedHighScoreName);
	pNode->GetChildValue("Guid", s);
	loadingProfile->m_sGuid = s;
	pNode->GetChildValue("SortOrder", s);	loadingProfile->m_SortOrder = StringToSortOrder(s);
	pNode->GetChildValue("LastDifficulty", s);	loadingProfile->m_LastDifficulty = StringToDifficulty(s);
	pNode->GetChildValue("LastStepsType", s);	loadingProfile->m_LastStepsType = GAMEMAN->StringToStepsType(s);
	pTemp = pNode->GetChild("Song");				if (pTemp) loadingProfile->m_lastSong.LoadFromNode(pTemp);
	pNode->GetChildValue("CurrentCombo", loadingProfile->m_iCurrentCombo);
	pNode->GetChildValue("TotalSessions", loadingProfile->m_iTotalSessions);
	pNode->GetChildValue("TotalSessionSeconds", loadingProfile->m_iTotalSessionSeconds);
	pNode->GetChildValue("TotalGameplaySeconds", loadingProfile->m_iTotalGameplaySeconds);
	pNode->GetChildValue("LastPlayedMachineGuid", loadingProfile->m_sLastPlayedMachineGuid);
	pNode->GetChildValue("LastPlayedDate", s); loadingProfile->m_LastPlayedDate.FromString(s);
	pNode->GetChildValue("TotalDancePoints", loadingProfile->m_iTotalDancePoints);
	pNode->GetChildValue("NumExtraStagesPassed", loadingProfile->m_iNumExtraStagesPassed);
	pNode->GetChildValue("NumExtraStagesFailed", loadingProfile->m_iNumExtraStagesFailed);
	pNode->GetChildValue("NumToasties", loadingProfile->m_iNumToasties);
	pNode->GetChildValue("TotalTapsAndHolds", loadingProfile->m_iTotalTapsAndHolds);
	pNode->GetChildValue("TotalJumps", loadingProfile->m_iTotalJumps);
	pNode->GetChildValue("TotalHolds", loadingProfile->m_iTotalHolds);
	pNode->GetChildValue("TotalRolls", loadingProfile->m_iTotalRolls);
	pNode->GetChildValue("TotalMines", loadingProfile->m_iTotalMines);
	pNode->GetChildValue("TotalHands", loadingProfile->m_iTotalHands);
	pNode->GetChildValue("TotalLifts", loadingProfile->m_iTotalLifts);
	pNode->GetChildValue("PlayerRating", loadingProfile->m_fPlayerRating);

	{
		const XNode* pDefaultModifiers = pNode->GetChild("DefaultModifiers");
		if (pDefaultModifiers)
		{
			FOREACH_CONST_Child(pDefaultModifiers, game_type)
			{
				game_type->GetTextValue(loadingProfile->m_sDefaultModifiers[game_type->GetName()]);
			}
		}
	}

	{
		const XNode* pFavorites = pNode->GetChild("Favorites");
		if (pFavorites) {
			FOREACH_CONST_Child(pFavorites, ck) {
				RString tmp = ck->GetName();				// handle duplicated entries caused by an oversight - mina
				bool duplicated = false;
				FOREACHS(string, loadingProfile->FavoritedCharts, chartkey)
					if (*chartkey == tmp)
						duplicated = true;
				if (!duplicated)
					loadingProfile->FavoritedCharts.emplace(tmp);
			}
			SONGMAN->SetFavoritedStatus(loadingProfile->FavoritedCharts);
		}
	}

	{
		const XNode* pPlayerSkillsets = pNode->GetChild("PlayerSkillsets");
		if (pPlayerSkillsets) {
			FOREACH_ENUM(Skillset, ss)
				pPlayerSkillsets->GetChildValue(SkillsetToString(ss), loadingProfile->m_fPlayerSkillsets[ss]);
		}
	}

	{
		const XNode* pNumSongsPlayedByPlayMode = pNode->GetChild("NumSongsPlayedByPlayMode");
		if (pNumSongsPlayedByPlayMode)
			FOREACH_ENUM(PlayMode, pm)
			pNumSongsPlayedByPlayMode->GetChildValue(PlayModeToString(pm), loadingProfile->m_iNumSongsPlayedByPlayMode[pm]);
	}

	{
		const XNode* pNumSongsPlayedByStyle = pNode->GetChild("NumSongsPlayedByStyle");
		if (pNumSongsPlayedByStyle)
		{
			FOREACH_CONST_Child(pNumSongsPlayedByStyle, style)
			{
				if (style->GetName() != "Style")
					continue;

				StyleID sID;
				sID.LoadFromNode(style);

				if (!sID.IsValid())
					WARN_AND_CONTINUE;

				style->GetTextValue(loadingProfile->m_iNumSongsPlayedByStyle[sID]);
			}
		}
	}

	{
		const XNode* pNumSongsPlayedByDifficulty = pNode->GetChild("NumSongsPlayedByDifficulty");
		if (pNumSongsPlayedByDifficulty)
			FOREACH_ENUM(Difficulty, dc)
			pNumSongsPlayedByDifficulty->GetChildValue(DifficultyToString(dc), loadingProfile->m_iNumSongsPlayedByDifficulty[dc]);
	}

	{
		const XNode* pNumSongsPlayedByMeter = pNode->GetChild("NumSongsPlayedByMeter");
		if (pNumSongsPlayedByMeter)
			for (int i = 0; i<MAX_METER + 1; i++)
				pNumSongsPlayedByMeter->GetChildValue(ssprintf("Meter%d", i), loadingProfile->m_iNumSongsPlayedByMeter[i]);
	}

	pNode->GetChildValue("NumTotalSongsPlayed", loadingProfile->m_iNumTotalSongsPlayed);

	{
		const XNode* pNumStagesPassedByGrade = pNode->GetChild("NumStagesPassedByGrade");
		if (pNumStagesPassedByGrade)
			FOREACH_ENUM(Grade, g)
			pNumStagesPassedByGrade->GetChildValue(GradeToString(g), loadingProfile->m_iNumStagesPassedByGrade[g]);
	}

	{
		const XNode* pNumStagesPassedByPlayMode = pNode->GetChild("NumStagesPassedByPlayMode");
		if (pNumStagesPassedByPlayMode)
			FOREACH_ENUM(PlayMode, pm)
			pNumStagesPassedByPlayMode->GetChildValue(PlayModeToString(pm), loadingProfile->m_iNumStagesPassedByPlayMode[pm]);

	}

	const XNode *pUserTable = pNode->GetChild("UserTable");

	Lua *L = LUA->Get();

	// If we have custom data, load it. Otherwise, make a blank table.
	if (pUserTable)
		LuaHelpers::CreateTableFromXNode(L, pUserTable);
	else
		lua_newtable(L);

	loadingProfile->m_UserTable.SetFromStack(L);
	LUA->Release(L);

}


void XMLProfile::LoadEttGeneralDataFromNode(const XNode* pNode) {
	CHECKPOINT_M("Loading the general node.");
	ASSERT(pNode->GetName() == "GeneralData");

	RString s;
	const XNode* pTemp;

	pNode->GetChildValue("DisplayName", loadingProfile->m_sDisplayName);
	pNode->GetChildValue("CharacterID", loadingProfile->m_sCharacterID);
	pNode->GetChildValue("LastUsedHighScoreName", loadingProfile->m_sLastUsedHighScoreName);
	pNode->GetChildValue("Guid", (*loadingProfile->GetGuid()));
	pNode->GetChildValue("SortOrder", s);	loadingProfile->m_SortOrder = StringToSortOrder(s);
	pNode->GetChildValue("LastDifficulty", s);	loadingProfile->m_LastDifficulty = StringToDifficulty(s);
	pNode->GetChildValue("LastStepsType", s);	loadingProfile->m_LastStepsType = GAMEMAN->StringToStepsType(s);
	pTemp = pNode->GetChild("Song");				if (pTemp) loadingProfile->m_lastSong.LoadFromNode(pTemp);
	pNode->GetChildValue("CurrentCombo", loadingProfile->m_iCurrentCombo);
	pNode->GetChildValue("TotalSessions", loadingProfile->m_iTotalSessions);
	pNode->GetChildValue("TotalSessionSeconds", loadingProfile->m_iTotalSessionSeconds);
	pNode->GetChildValue("TotalGameplaySeconds", loadingProfile->m_iTotalGameplaySeconds);
	pNode->GetChildValue("LastPlayedDate", s); loadingProfile->m_LastPlayedDate.FromString(s);
	pNode->GetChildValue("TotalDancePoints", loadingProfile->m_iTotalDancePoints);
	pNode->GetChildValue("NumToasties", loadingProfile->m_iNumToasties);
	pNode->GetChildValue("TotalTapsAndHolds", loadingProfile->m_iTotalTapsAndHolds);
	pNode->GetChildValue("TotalJumps", loadingProfile->m_iTotalJumps);
	pNode->GetChildValue("TotalHolds", loadingProfile->m_iTotalHolds);
	pNode->GetChildValue("TotalRolls", loadingProfile->m_iTotalRolls);
	pNode->GetChildValue("TotalMines", loadingProfile->m_iTotalMines);
	pNode->GetChildValue("TotalHands", loadingProfile->m_iTotalHands);
	pNode->GetChildValue("TotalLifts", loadingProfile->m_iTotalLifts);
	pNode->GetChildValue("PlayerRating", loadingProfile->m_fPlayerRating);

	{
		const XNode* pDefaultModifiers = pNode->GetChild("DefaultModifiers");
		if (pDefaultModifiers)
		{
			FOREACH_CONST_Child(pDefaultModifiers, game_type)
			{
				game_type->GetTextValue(loadingProfile->m_sDefaultModifiers[game_type->GetName()]);
			}
		}
	}

	{
		const XNode* pPlayerSkillsets = pNode->GetChild("PlayerSkillsets");
		if (pPlayerSkillsets) {
			FOREACH_ENUM(Skillset, ss)
				pPlayerSkillsets->GetChildValue(SkillsetToString(ss), loadingProfile->m_fPlayerSkillsets[ss]);
		}
	}

	const XNode *pUserTable = pNode->GetChild("UserTable");

	Lua *L = LUA->Get();

	// If we have custom data, load it. Otherwise, make a blank table.
	if (pUserTable)
		LuaHelpers::CreateTableFromXNode(L, pUserTable);
	else
		lua_newtable(L);

	loadingProfile->m_UserTable.SetFromStack(L);
	LUA->Release(L);

}

XNode* XMLProfile::SaveSongScoresCreateNode(const Profile* profile) const
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
}


XNode* XMLProfile::SaveEttScoresCreateNode(const Profile* profile) const {
	CHECKPOINT_M("Saving the player scores node.");

	ASSERT(profile != NULL);
	XNode* pNode = SCOREMAN->CreateNode();
	return pNode;
}

void XMLProfile::LoadEttScoresFromNode(const XNode* pSongScores) {
	CHECKPOINT_M("Loading the player scores node.");
	SCOREMAN->LoadFromNode(pSongScores);
}


void XMLProfile::LoadScreenshotDataFromNode(const XNode* pScreenshotData)
{
	CHECKPOINT_M("Loading the node containing screenshot data.");

	ASSERT(pScreenshotData->GetName() == "ScreenshotData");
	FOREACH_CONST_Child(pScreenshotData, pScreenshot)
	{
		if (pScreenshot->GetName() != "Screenshot")
			WARN_AND_CONTINUE_M(pScreenshot->GetName());

		Screenshot ss;
		ss.LoadFromNode(pScreenshot);

		loadingProfile->m_vScreenshots.push_back(ss);
	}
}

XNode* XMLProfile::SaveScreenshotDataCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Getting the node containing screenshot data.");

	ASSERT(profile != NULL);

	XNode* pNode = new XNode("ScreenshotData");

	FOREACH_CONST(Screenshot, profile->m_vScreenshots, ss)
	{
		pNode->AppendChild(ss->CreateNode());
	}

	return pNode;
}

void XMLProfile::LoadCategoryScoresFromNode(const XNode* pCategoryScores)
{
	CHECKPOINT_M("Loading the node that contains category scores.");

	ASSERT(pCategoryScores->GetName() == "CategoryScores");

	FOREACH_CONST_Child(pCategoryScores, pStepsType)
	{
		if (pStepsType->GetName() != "StepsType")
			continue;

		RString str;
		if (!pStepsType->GetAttrValue("Type", str))
			WARN_AND_CONTINUE;
		StepsType st = GAMEMAN->StringToStepsType(str);
		if (st == StepsType_Invalid)
			WARN_AND_CONTINUE_M(str);

		FOREACH_CONST_Child(pStepsType, pRadarCategory)
		{
			if (pRadarCategory->GetName() != "RankingCategory")
				continue;

			if (!pRadarCategory->GetAttrValue("Type", str))
				WARN_AND_CONTINUE;
			RankingCategory rc = StringToRankingCategory(str);
			if (rc == RankingCategory_Invalid)
				WARN_AND_CONTINUE_M(str);

			const XNode *pHighScoreListNode = pRadarCategory->GetChild("HighScoreList");
			if (pHighScoreListNode == NULL)
				WARN_AND_CONTINUE;

			HighScoreList &hsl = loadingProfile->GetCategoryHighScoreList(st, rc);
			hsl.LoadFromNode(pHighScoreListNode);
		}
	}
}


XNode* XMLProfile::SaveCategoryScoresCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Getting the node that saves category scores.");

	ASSERT(profile != NULL);

	XNode* pNode = new XNode("CategoryScores");

	FOREACH_ENUM(StepsType, st)
	{
		// skip steps types that have never been played
		if (profile->GetCategoryNumTimesPlayed(st) == 0)
			continue;

		XNode* pStepsTypeNode = pNode->AppendChild("StepsType");
		pStepsTypeNode->AppendAttr("Type", GAMEMAN->GetStepsTypeInfo(st).szName);

		FOREACH_ENUM(RankingCategory, rc)
		{
			// skip steps types/categories that have never been played
			if (profile->GetCategoryHighScoreList(st, rc).GetNumTimesPlayed() == 0)
				continue;

			XNode* pRankingCategoryNode = pStepsTypeNode->AppendChild("RankingCategory");
			pRankingCategoryNode->AppendAttr("Type", RankingCategoryToString(rc));

			const HighScoreList &hsl = profile->GetCategoryHighScoreList((StepsType)st, (RankingCategory)rc);

			pRankingCategoryNode->AppendChild(hsl.CreateNode());
		}
	}

	return pNode;
}


ProfileLoadResult XMLProfile::LoadEttXmlFromNode(const XNode *xml) {
	/* The placeholder stats.xml file has an <html> tag. Don't load it,
	* but don't warn about it. */
	if (xml->GetName() == "html")
		return ProfileLoadResult_FailedNoProfile;

	if (xml->GetName() != "Stats")
	{
		WARN_M(xml->GetName());
		return ProfileLoadResult_FailedTampered;
	}

	const XNode* gen = xml->GetChild("GeneralData");
	if (gen)
		LoadEttGeneralDataFromNode(gen);

	const XNode* favs = xml->GetChild("Favorites");
	if (favs)
		LoadFavoritesFromNode(favs);

	const XNode* pmir = xml->GetChild("PermaMirror");
	if (pmir)
		LoadPermaMirrorFromNode(pmir);

	const XNode* goals = xml->GetChild("ScoreGoals");
	if (goals)
		LoadScoreGoalsFromNode(goals);

	const XNode* play = xml->GetChild("Playlists");
	if (play)
		LoadPlaylistsFromNode(play);

	const XNode* scores = xml->GetChild("PlayerScores");
	if (scores)
		LoadEttScoresFromNode(scores);

	return ProfileLoadResult_Success;
}

ProfileLoadResult XMLProfile::LoadStatsXmlFromNode(const XNode *xml, bool bIgnoreEditable)
{
	/* The placeholder stats.xml file has an <html> tag. Don't load it,
	* but don't warn about it. */
	if (xml->GetName() == "html")
		return ProfileLoadResult_FailedNoProfile;

	if (xml->GetName() != "Stats")
	{
		WARN_M(xml->GetName());
		return ProfileLoadResult_FailedTampered;
	}

	// These are loaded from Editable, so we usually want to ignore them here.
	RString sName = loadingProfile->m_sDisplayName;
	RString sCharacterID = loadingProfile->m_sCharacterID;
	RString sLastUsedHighScoreName = loadingProfile->m_sLastUsedHighScoreName;

	LOAD_NODE(GeneralData);
	LOAD_NODE(SongScores);
	LOAD_NODE(CategoryScores);
	LOAD_NODE(ScreenshotData);

	if (bIgnoreEditable)
	{
		loadingProfile->m_sDisplayName = sName;
		loadingProfile->m_sCharacterID = sCharacterID;
		loadingProfile->m_sLastUsedHighScoreName = sLastUsedHighScoreName;
	}

	return ProfileLoadResult_Success;
}


void XMLProfile::LoadSongScoresFromNode(const XNode* pSongScores)
{
	CHECKPOINT_M("Loading the node that contains song scores.");

	ASSERT(pSongScores->GetName() == "SongScores");

	FOREACH_CONST_Child(pSongScores, pSong)
	{
		if (pSong->GetName() != "Song")
			continue;

		SongID songID;
		songID.LoadFromNode(pSong);
		// Allow invalid songs so that scores aren't deleted for people that use
		// AdditionalSongsFolders and change it frequently. -Kyz
		//if( !songID.IsValid() )
		//	continue;

		FOREACH_CONST_Child(pSong, pSteps)
		{
			if (pSteps->GetName() != "Steps")
				continue;

			StepsID stepsID;
			stepsID.LoadFromNode(pSteps);
			if (!stepsID.IsValid())
				WARN_AND_CONTINUE;

			const XNode *pHighScoreListNode = pSteps->GetChild("HighScoreList");
			if (pHighScoreListNode == NULL)
				WARN_AND_CONTINUE;

			HighScoreList &hsl = loadingProfile->m_SongHighScores[songID].m_StepsHighScores[stepsID].hsl;
			hsl.LoadFromNode(pHighScoreListNode);
		}
	}
}


XNode *XMLProfile::SaveStatsXmlCreateNode(const Profile* profile) const
{
	XNode *xml = new XNode("Stats");

	xml->AppendChild(SaveGeneralDataCreateNode(profile));
	xml->AppendChild(SaveSongScoresCreateNode(profile));
	xml->AppendChild(SaveCategoryScoresCreateNode(profile));
	xml->AppendChild(SaveScreenshotDataCreateNode(profile));

	return xml;
}

XNode *XMLProfile::SaveEttXmlCreateNode(const Profile* profile) const
{
	XNode *xml = new XNode("Stats");
	xml->AppendChild(SaveEttGeneralDataCreateNode(profile));

	if (!profile->FavoritedCharts.empty())
		xml->AppendChild(SaveFavoritesCreateNode(profile));

	if (!profile->PermaMirrorCharts.empty())
		xml->AppendChild(SavePermaMirrorCreateNode(profile));

	if (!SONGMAN->allplaylists.empty())
		xml->AppendChild(SavePlaylistsCreateNode(profile));

	if (!profile->goalmap.empty())
		xml->AppendChild(SaveScoreGoalsCreateNode(profile));

	xml->AppendChild(SaveEttScoresCreateNode(profile));
	return xml;
}
