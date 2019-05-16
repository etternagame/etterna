
#include "Etterna/Globals/global.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "Profile.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "XMLProfile.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileDriverDeflate.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "RageUtil/File/RageFileManager.h"

#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

const RString ETT_XML = "Etterna.xml";
const RString ETT_XML_GZ = "Etterna.xml.gz";
/** @brief The filename containing the signature for ETT_XML's signature. */
const RString DONT_SHARE_SIG = "DontShare.sig";
static Preference<bool> g_bProfileDataCompress("ProfileDataCompress", false);

// Loading and saving
#define WARN_PARSER                                                            \
	ShowWarningOrTrace(__FILE__, __LINE__, "Error parsing file.", true)
#define WARN_AND_RETURN                                                        \
	{                                                                          \
		WARN_PARSER;                                                           \
		return;                                                                \
	}
#define WARN_AND_CONTINUE                                                      \
	{                                                                          \
		WARN_PARSER;                                                           \
		continue;                                                              \
	}
#define WARN_AND_BREAK                                                         \
	{                                                                          \
		WARN_PARSER;                                                           \
		break;                                                                 \
	}
#define WARN_M(m)                                                              \
	ShowWarningOrTrace(                                                        \
	  __FILE__, __LINE__, RString("Error parsing file: ") + (m), true)
#define WARN_AND_RETURN_M(m)                                                   \
	{                                                                          \
		WARN_M(m);                                                             \
		return;                                                                \
	}
#define WARN_AND_CONTINUE_M(m)                                                 \
	{                                                                          \
		WARN_M(m);                                                             \
		continue;                                                              \
	}
#define WARN_AND_BREAK_M(m)                                                    \
	{                                                                          \
		WARN_M(m);                                                             \
		break;                                                                 \
	}

#define LOAD_NODE(X)                                                           \
	{                                                                          \
		const XNode* X = xml->GetChild(#X);                                    \
		if (X == NULL)                                                         \
			LOG->Warn("Failed to read section " #X);                           \
		else                                                                   \
			Load##X##FromNode(X);                                              \
	}


ProfileLoadResult
XMLProfile::LoadEttFromDir(RString dir)
{
	dir += PROFILEMAN->GetStatsPrefix();
	profiledir = dir;
	loadingProfile->IsEtternaProfile = true;
	RString fn = dir + ETT_XML;

	int iError;
	unique_ptr<RageFileBasic> pFile(FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == NULL) {
		LOG->Trace("Error opening %s: %s", fn.c_str(), strerror(iError));
		return ProfileLoadResult_FailedTampered;
	}

	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("Loading %s", fn.c_str());
	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return ProfileLoadResult_FailedTampered;
	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("Done.");

	return LoadEttXmlFromNode(&xml);
}

bool
XMLProfile::SaveEttXmlToDir(RString sDir, const Profile* profile) const
{
	LOG->Trace("Saving Etterna Profile to: %s", sDir.c_str());
	unique_ptr<XNode> xml(SaveEttXmlCreateNode(profile));
	sDir += PROFILEMAN->GetStatsPrefix();
	// Save Etterna.xml
	RString fn = sDir + ETT_XML;
	{
		RString sError;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE)) {
			LuaHelpers::ReportScriptErrorFmt("Couldn't open %s for writing: %s",
											 fn.c_str(),
											 f.GetError().c_str());
			return false;
		}

		if (g_bProfileDataCompress) {
			RageFileObjGzip gzip(&f);
			gzip.Start();
			if (!XmlFileUtil::SaveToFile(xml.get(), gzip, "", false))
				return false;

			if (gzip.Finish() == -1)
				return false;

			/* After successfully saving ETT_XML_GZ, remove any stray
			 * ETT_XML. */
			if (FILEMAN->IsAFile(sDir + ETT_XML))
				FILEMAN->Remove(sDir + ETT_XML);
		} else {
			if (!XmlFileUtil::SaveToFile(xml.get(), f, "", false))
				return false;

			/* After successfully saving ETT_XML, remove any stray
			 * ETT_XML_GZ. */
			if (FILEMAN->IsAFile(sDir + ETT_XML_GZ))
				FILEMAN->Remove(sDir + ETT_XML_GZ);
		}
	}

	return true;
}

XNode*
XMLProfile::SaveFavoritesCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Saving the favorites node.");

	XNode* favs = new XNode("Favorites");
	for(auto const it : profile->FavoritedCharts)
	    favs->AppendChild(it);
	return favs;
}

XNode*
XMLProfile::SavePermaMirrorCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Saving the permamirror node.");

	XNode* pmir = new XNode("PermaMirror");
	for(auto const it : profile->PermaMirrorCharts)
	    pmir->AppendChild(it);
	return pmir;
}

XNode*
GoalsForChart::CreateNode() const
{
	XNode* cg = new XNode("GoalsForChart");

	if (!goals.empty()) {
		cg->AppendAttr("Key", goals[0].chartkey);
		for(auto const sg : goals)
		    cg->AppendChild(sg.CreateNode());
	}
	return cg;
}

XNode*
XMLProfile::SaveScoreGoalsCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Saving the scoregoals node.");

	XNode* goals = new XNode("ScoreGoals");
	for(auto i : profile->goalmap){
		const GoalsForChart& cg = i.second;
		goals->AppendChild(cg.CreateNode());
	}
	return goals;
}

XNode*
XMLProfile::SavePlaylistsCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Saving the playlists node.");

	XNode* playlists = new XNode("Playlists");
	auto& pls = profile->allplaylists;
	for(auto const i : pls)
        if (i.first != "" && i.first != "Favorites")
            playlists->AppendChild(i.second.CreateNode());
	return playlists;
}

void
XMLProfile::LoadFavoritesFromNode(const XNode* pNode)
{
	CHECKPOINT_M("Loading the favorites node.");

	FOREACH_CONST_Child(pNode, ck) loadingProfile->FavoritedCharts.emplace(ck->GetName());
	SONGMAN->SetFavoritedStatus(loadingProfile->FavoritedCharts);
	SONGMAN->MakePlaylistFromFavorites(loadingProfile->FavoritedCharts,
									   loadingProfile->allplaylists);
}

void
XMLProfile::LoadPermaMirrorFromNode(const XNode* pNode)
{
	CHECKPOINT_M("Loading the permamirror node.");

	FOREACH_CONST_Child(pNode, ck) loadingProfile->PermaMirrorCharts.emplace(ck->GetName());
	SONGMAN->SetPermaMirroredStatus(loadingProfile->PermaMirrorCharts);
}

void
GoalsForChart::LoadFromNode(const XNode* pNode)
{
	FOREACH_CONST_Child(pNode, sg)
	{
		ScoreGoal doot;
		doot.LoadFromNode(sg);
		Add(doot);
	}
	RString chartkey;
	pNode->GetAttrValue("Key", chartkey);
	for (auto& goal : goals)
		goal.chartkey = chartkey;
}

void
XMLProfile::LoadScoreGoalsFromNode(const XNode* pNode)
{
	CHECKPOINT_M("Loading the scoregoals node.");

	RString ck;
	FOREACH_CONST_Child(pNode, chgoals)
	{
		chgoals->GetAttrValue("Key", ck);
		loadingProfile->goalmap[ck].LoadFromNode(chgoals);

		// this should load using the chart system but ensure keys are set
		// properly here for now -mina
		for (auto& sg : loadingProfile->goalmap[ck].goals)
			sg.chartkey = ck;
	}
}

void
XMLProfile::LoadPlaylistsFromNode(const XNode* pNode)
{
	CHECKPOINT_M("Loading the playlists node.");

	auto& pls = loadingProfile->allplaylists;
	FOREACH_CONST_Child(pNode, pl)
	{
		Playlist tmp;
		tmp.LoadFromNode(pl);
		pls.emplace(tmp.name, tmp);
		SONGMAN->activeplaylist = tmp.name;
	}
}

XNode*
XMLProfile::SaveEttGeneralDataCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Saving the general node.");

	XNode* pGeneralDataNode = new XNode("GeneralData");

	// TRICKY: These are write-only elements that are normally never read again.
	// This data is required by other apps (like internet ranking), but is
	// redundant to the game app.
	pGeneralDataNode->AppendChild("DisplayName",
								  profile->GetDisplayNameOrHighScoreName());
	pGeneralDataNode->AppendChild("CharacterID", profile->m_sCharacterID);
	pGeneralDataNode->AppendChild("Guid", profile->m_sGuid);
	pGeneralDataNode->AppendChild("SortOrder",
								  SortOrderToString(profile->m_SortOrder));

	if (profile->m_LastDifficulty <
		0) // force set difficulty to current steps if this is somehow -1 for
		   // ??? reasons -mina
		pGeneralDataNode->AppendChild(
		  "LastDifficulty",
		  DifficultyToString(
			GAMESTATE->m_pCurSteps->GetDifficulty()));
	else if (profile->m_LastDifficulty < Difficulty_Invalid)
		pGeneralDataNode->AppendChild(
		  "LastDifficulty", DifficultyToString(profile->m_LastDifficulty));
	if (profile->m_LastStepsType != StepsType_Invalid)
		pGeneralDataNode->AppendChild(
		  "LastStepsType",
		  GAMEMAN->GetStepsTypeInfo(profile->m_LastStepsType).szName);
	pGeneralDataNode->AppendChild(profile->m_lastSong.CreateNode());
	pGeneralDataNode->AppendChild("TotalSessions", profile->m_iTotalSessions);
	pGeneralDataNode->AppendChild("TotalSessionSeconds",
								  profile->m_iTotalSessionSeconds);
	pGeneralDataNode->AppendChild("TotalGameplaySeconds",
								  profile->m_iTotalGameplaySeconds);
	pGeneralDataNode->AppendChild("LastPlayedMachineGuid",
								  profile->m_sLastPlayedMachineGuid);
	pGeneralDataNode->AppendChild("LastPlayedDate",
								  profile->m_LastPlayedDate.GetString());
	pGeneralDataNode->AppendChild("TotalDancePoints",
								  profile->m_iTotalDancePoints);
	pGeneralDataNode->AppendChild("NumToasties", profile->m_iNumToasties);
	pGeneralDataNode->AppendChild("TotalTapsAndHolds",
								  profile->m_iTotalTapsAndHolds);
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
		XNode* pDefaultModifiers =
		  pGeneralDataNode->AppendChild("DefaultModifiers");
		for(auto const &it : profile->m_sDefaultModifiers)
		    pDefaultModifiers->AppendChild(it.first, it.second);
	}

	{
		XNode* pPlayerSkillsets =
		  pGeneralDataNode->AppendChild("PlayerSkillsets");
		FOREACH_ENUM(Skillset, ss)
		pPlayerSkillsets->AppendChild(SkillsetToString(ss),
									  profile->m_fPlayerSkillsets[ss]);
	}

	pGeneralDataNode->AppendChild("NumTotalSongsPlayed",
								  profile->m_iNumTotalSongsPlayed);

	{
		XNode* pNumStagesPassedByPlayMode =
		  pGeneralDataNode->AppendChild("NumStagesPassedByPlayMode");
		FOREACH_ENUM(PlayMode, pm)
		{
			// Don't save unplayed PlayModes.
			if (!profile->m_iNumStagesPassedByPlayMode[pm])
				continue;
			pNumStagesPassedByPlayMode->AppendChild(
			  PlayModeToString(pm), profile->m_iNumStagesPassedByPlayMode[pm]);
		}
	}

	// Load Lua UserTable from profile
	if (profile->m_UserTable.IsSet()) {
		Lua* L = LUA->Get();
		profile->m_UserTable.PushSelf(L);
		XNode* pUserTable = XmlFileUtil::XNodeFromTable(L);
		LUA->Release(L);

		// XXX: XNodeFromTable returns a root node with the name "Layer".
		pUserTable->m_sName = "UserTable";
		pGeneralDataNode->AppendChild(pUserTable);
	}

	return pGeneralDataNode;
}

void
XMLProfile::MoveBackupToDir(const RString& sFromDir, const RString& sToDir)
{
	if (FILEMAN->IsAFile(sFromDir + ETT_XML) &&
		FILEMAN->IsAFile(sFromDir + ETT_XML + SIGNATURE_APPEND)) {
		FILEMAN->Move(sFromDir + ETT_XML, sToDir + ETT_XML);
		FILEMAN->Move(sFromDir + ETT_XML + SIGNATURE_APPEND,
					  sToDir + ETT_XML + SIGNATURE_APPEND);
	} else if (FILEMAN->IsAFile(sFromDir + ETT_XML_GZ) &&
			   FILEMAN->IsAFile(sFromDir + ETT_XML_GZ + SIGNATURE_APPEND)) {
		FILEMAN->Move(sFromDir + ETT_XML_GZ, sToDir + ETT_XML);
		FILEMAN->Move(sFromDir + ETT_XML_GZ + SIGNATURE_APPEND,
					  sToDir + ETT_XML + SIGNATURE_APPEND);
	}
}


void
XMLProfile::LoadEttGeneralDataFromNode(const XNode* pNode)
{
	CHECKPOINT_M("Loading the general node.");
	ASSERT(pNode->GetName() == "GeneralData");

	RString s;
	const XNode* pTemp;

	pNode->GetChildValue("DisplayName", loadingProfile->m_sDisplayName);
	pNode->GetChildValue("CharacterID", loadingProfile->m_sCharacterID);
	pNode->GetChildValue("LastUsedHighScoreName",
						 loadingProfile->m_sLastUsedHighScoreName);
	pNode->GetChildValue("Guid", (*loadingProfile->GetGuid()));
	pNode->GetChildValue("SortOrder", s);
	loadingProfile->m_SortOrder = StringToSortOrder(s);
	pNode->GetChildValue("LastDifficulty", s);
	loadingProfile->m_LastDifficulty = StringToDifficulty(s);
	pNode->GetChildValue("LastStepsType", s);
	loadingProfile->m_LastStepsType = GAMEMAN->StringToStepsType(s);
	pTemp = pNode->GetChild("Song");
	if (pTemp)
		loadingProfile->m_lastSong.LoadFromNode(pTemp);
	pNode->GetChildValue("CurrentCombo", loadingProfile->m_iCurrentCombo);
	pNode->GetChildValue("TotalSessions", loadingProfile->m_iTotalSessions);
	pNode->GetChildValue("TotalSessionSeconds",
						 loadingProfile->m_iTotalSessionSeconds);
	pNode->GetChildValue("TotalGameplaySeconds",
						 loadingProfile->m_iTotalGameplaySeconds);
	pNode->GetChildValue("LastPlayedDate", s);
	loadingProfile->m_LastPlayedDate.FromString(s);
	pNode->GetChildValue("TotalDancePoints",
						 loadingProfile->m_iTotalDancePoints);
	pNode->GetChildValue("NumToasties", loadingProfile->m_iNumToasties);
	pNode->GetChildValue("TotalTapsAndHolds",
						 loadingProfile->m_iTotalTapsAndHolds);
	pNode->GetChildValue("TotalJumps", loadingProfile->m_iTotalJumps);
	pNode->GetChildValue("TotalHolds", loadingProfile->m_iTotalHolds);
	pNode->GetChildValue("TotalRolls", loadingProfile->m_iTotalRolls);
	pNode->GetChildValue("TotalMines", loadingProfile->m_iTotalMines);
	pNode->GetChildValue("TotalHands", loadingProfile->m_iTotalHands);
	pNode->GetChildValue("TotalLifts", loadingProfile->m_iTotalLifts);
	pNode->GetChildValue("PlayerRating", loadingProfile->m_fPlayerRating);

	{
		const XNode* pDefaultModifiers = pNode->GetChild("DefaultModifiers");
		if (pDefaultModifiers) {
			FOREACH_CONST_Child(pDefaultModifiers, game_type)
			{
				game_type->GetTextValue(
				  loadingProfile->m_sDefaultModifiers[game_type->GetName()]);
			}
		}
	}

	{
		const XNode* pPlayerSkillsets = pNode->GetChild("PlayerSkillsets");
		if (pPlayerSkillsets) {
			FOREACH_ENUM(Skillset, ss)
			pPlayerSkillsets->GetChildValue(
			  SkillsetToString(ss), loadingProfile->m_fPlayerSkillsets[ss]);
		}
	}

	const XNode* pUserTable = pNode->GetChild("UserTable");

	Lua* L = LUA->Get();

	// If we have custom data, load it. Otherwise, make a blank table.
	if (pUserTable)
		LuaHelpers::CreateTableFromXNode(L, pUserTable);
	else
		lua_newtable(L);

	loadingProfile->m_UserTable.SetFromStack(L);
	LUA->Release(L);
}

XNode*
XMLProfile::SaveEttScoresCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Saving the player scores node.");

	ASSERT(profile != NULL);
	SCOREMAN->SetAllTopScores(profile->m_sProfileID);
	XNode* pNode = SCOREMAN->CreateNode(profile->m_sProfileID);
	return pNode;
}

void
XMLProfile::LoadEttScoresFromNode(const XNode* pSongScores)
{
	CHECKPOINT_M("Loading the player scores node.");
	SCOREMAN->LoadFromNode(pSongScores, loadingProfile->m_sProfileID);
}

void
XMLProfile::LoadScreenshotDataFromNode(const XNode* pScreenshotData)
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

XNode*
XMLProfile::SaveScreenshotDataCreateNode(const Profile* profile) const
{
	CHECKPOINT_M("Getting the node containing screenshot data.");

	ASSERT(profile != NULL);

	XNode* pNode = new XNode("ScreenshotData");

	for(auto const ss : profile->m_vScreenshots)
		pNode->AppendChild(ss.CreateNode());

	return pNode;
}

ProfileLoadResult
XMLProfile::LoadEttXmlFromNode(const XNode* xml)
{
	/* The placeholder stats.xml file has an <html> tag. Don't load it,
	 * but don't warn about it. */
	if (xml->GetName() == "html")
		return ProfileLoadResult_FailedNoProfile;

	if (xml->GetName() != "Stats") {
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


XNode*
XMLProfile::SaveEttXmlCreateNode(const Profile* profile) const
{
	XNode* xml = new XNode("Stats");
	xml->AppendChild(SaveEttGeneralDataCreateNode(profile));

	if (!profile->FavoritedCharts.empty())
		xml->AppendChild(SaveFavoritesCreateNode(profile));

	if (!profile->PermaMirrorCharts.empty())
		xml->AppendChild(SavePermaMirrorCreateNode(profile));

	if (!profile->allplaylists.empty())
		xml->AppendChild(SavePlaylistsCreateNode(profile));

	if (!profile->goalmap.empty())
		xml->AppendChild(SaveScoreGoalsCreateNode(profile));

	xml->AppendChild(SaveEttScoresCreateNode(profile));
	return xml;
}
