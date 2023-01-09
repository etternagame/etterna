#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "GameState.h"
#include "Etterna/FileTypes/IniFile.h"
#include "NoteSkinManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "ThemeManager.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "arch/Dialog/Dialog.h"

#include <map>

using std::map;

/** @brief Have the NoteSkinManager available throughout the program. */
NoteSkinManager* NOTESKIN =
  nullptr; // global and accessible from anywhere in our program

const std::string GAME_COMMON_NOTESKIN_NAME = "common";
const std::string GAME_BASE_NOTESKIN_NAME = "default";

// this isn't a global because of nondeterministic global actor ordering
// might init this before SpecialFiles::NOTESKINS_DIR
#define GLOBAL_BASE_DIR                                                        \
	(SpecialFiles::NOTESKINS_DIR + GAME_COMMON_NOTESKIN_NAME + "/")

static map<std::string, std::string> g_PathCache;

struct NoteSkinData
{
	std::string sName;
	IniFile metrics;

	// When looking for an element, search these dirs from head to tail.
	std::vector<std::string> vsDirSearchOrder;

	LuaReference m_Loader;
};

namespace {
static map<std::string, NoteSkinData> g_mapNameToData;
} // namespace;

NoteSkinManager::NoteSkinManager()
{
	m_pCurGame = nullptr;
	m_PlayerNumber = PlayerNumber_Invalid;
	m_GameController = GameController_Invalid;

	// Register with Lua.
	{
		auto L = LUA->Get();
		lua_pushstring(L, "NOTESKIN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

NoteSkinManager::~NoteSkinManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("NOTESKIN");

	g_mapNameToData.clear();
}

void
NoteSkinManager::RefreshNoteSkinData(const Game* pGame)
{
	m_pCurGame = pGame;

	// clear path cache
	g_PathCache.clear();

	std::string gameName = pGame->m_szName;
	// how to make solo use dance skins
	if (gameName == "solo")
		gameName = "dance";
	auto sBaseSkinFolder = SpecialFiles::NOTESKINS_DIR + gameName + "/";
	std::string sGlobalSkinFolder =
	  SpecialFiles::NOTESKINS_DIR + "global" + "/";
	auto sThemeSkinFolder =
	  THEME->GetCurThemeDir() + "/NoteSkins/" + gameName + "/";
	std::vector<std::string> asNoteSkinNames;
	GetDirListing(sBaseSkinFolder + "*", asNoteSkinNames, true);
	GetDirListing(sGlobalSkinFolder + "*", asNoteSkinNames, true);
	GetDirListing(sThemeSkinFolder + "*", asNoteSkinNames, true);

	g_mapNameToData.clear();
	for (unsigned j = 0; j < asNoteSkinNames.size(); j++) {
		auto sName = make_lower(asNoteSkinNames[j]);
		// Don't feel like changing the structure of this code to load the skin
		// into a temp variable and move it, so if the load fails, then just
		// delete it from the map. -Kyz
		if (!LoadNoteSkinData(sName, g_mapNameToData[sName])) {
			auto entry = g_mapNameToData.find(sName);
			g_mapNameToData.erase(entry);
		}
	}
}

bool
NoteSkinManager::LoadNoteSkinData(const std::string& sNoteSkinName,
								  NoteSkinData& data_out)
{
	data_out.sName = sNoteSkinName;
	data_out.metrics.Clear();
	data_out.vsDirSearchOrder.clear();

	// Read the current NoteSkin and all of its fallbacks
	return LoadNoteSkinDataRecursive(sNoteSkinName, data_out);
}

bool
NoteSkinManager::LoadNoteSkinDataRecursive(const std::string& sNoteSkinName_,
										   NoteSkinData& data_out)
{
	auto sNoteSkinName(sNoteSkinName_);

	auto iDepth = 0;
	auto bLoadedCommon = false;
	auto bLoadedBase = false;
	std::string gameName = m_pCurGame->m_szName;
	// how to make solo use dance noteskins
	if (gameName == "solo")
		gameName = "dance";
	for (;;) {
		++iDepth;
		if (iDepth >= 20) {
			LuaHelpers::ReportScriptError(
			  "Circular NoteSkin fallback references detected.",
			  "NOTESKIN_ERROR");
			return false;
		}

		auto sDir =
		  SpecialFiles::NOTESKINS_DIR + gameName + "/" + sNoteSkinName + "/";
		if (!FILEMAN->IsADirectory(sDir))
			sDir = SpecialFiles::NOTESKINS_DIR + "global" + "/" +
				   sNoteSkinName + "/";

		if (!FILEMAN->IsADirectory(sDir))
			sDir = THEME->GetCurThemeDir() + "/NoteSkins/" + gameName + "/" +
				   sNoteSkinName + "/";

		if (!FILEMAN->IsADirectory(sDir)) {
			sDir = GLOBAL_BASE_DIR + sNoteSkinName + "/";
			if (!FILEMAN->IsADirectory(sDir)) {
				LuaHelpers::ReportScriptError(
				  "NoteSkin \"" + data_out.sName + "\" references skin \"" +
					sNoteSkinName + "\" that is not present",
				  "NOTESKIN_ERROR");
				return false;
			}
		}

		Locator::getLogger()->debug("LoadNoteSkinDataRecursive: {} ({})",
					sNoteSkinName.c_str(),sDir.c_str());

		// read global fallback the current NoteSkin (if any)
		IniFile ini;
		ini.ReadFile(sDir + "metrics.ini");

		if (!CompareNoCase(sNoteSkinName, GAME_BASE_NOTESKIN_NAME))
			bLoadedBase = true;
		if (!CompareNoCase(sNoteSkinName, GAME_COMMON_NOTESKIN_NAME))
			bLoadedCommon = true;

		std::string sFallback;
		if (!ini.GetValue("Global", "FallbackNoteSkin", sFallback)) {
			if (!bLoadedBase)
				sFallback = GAME_BASE_NOTESKIN_NAME;
			else if (!bLoadedCommon)
				sFallback = GAME_COMMON_NOTESKIN_NAME;
		}

		XmlFileUtil::MergeIniUnder(&ini, &data_out.metrics);

		data_out.vsDirSearchOrder.push_back(sDir);

		if (sFallback.empty())
			break;
		sNoteSkinName = sFallback;
	}

	LuaReference refScript;
	for (auto dir = data_out.vsDirSearchOrder.rbegin();
		 dir != data_out.vsDirSearchOrder.rend();
		 ++dir) {
		auto sFile = *dir + "NoteSkin.lua";
		std::string sScript;
		if (!FILEMAN->IsAFile(sFile))
			continue;

		if (!GetFileContents(sFile, sScript))
			continue;

		Locator::getLogger()->trace("Load script \"{}\"", sFile.c_str());

		auto L = LUA->Get();
		auto Error = "Error running " + sFile + ": ";
		refScript.PushSelf(L);
		if (!LuaHelpers::RunScript(
			  L, sScript, "@" + sFile, Error, 1, 1, true)) {
			lua_pop(L, 1);
		} else {
			refScript.SetFromStack(L);
		}
		LUA->Release(L);
	}
	data_out.m_Loader = refScript;
	return true;
}

void
NoteSkinManager::GetNoteSkinNames(std::vector<std::string>& AddTo)
{
	GetNoteSkinNames(GAMESTATE->m_pCurGame, AddTo);
}

void
NoteSkinManager::GetNoteSkinNames(const Game* pGame, std::vector<std::string>& AddTo)
{
	GetAllNoteSkinNamesForGame(pGame, AddTo);
}

bool
NoteSkinManager::NoteSkinNameInList(const std::string& name,
									const std::vector<std::string>& name_list)
{
	for (size_t i = 0; i < name_list.size(); ++i) {
		if (0 == strcasecmp(name.c_str(), name_list[i].c_str())) {
			return true;
		}
	}
	return false;
}

bool
NoteSkinManager::DoesNoteSkinExist(const std::string& sSkinName)
{
	std::vector<std::string> asSkinNames;
	GetAllNoteSkinNamesForGame(GAMESTATE->m_pCurGame, asSkinNames);
	return NoteSkinNameInList(sSkinName, asSkinNames);
}

bool
NoteSkinManager::DoNoteSkinsExistForGame(const Game* pGame)
{
	std::vector<std::string> asSkinNames;
	GetAllNoteSkinNamesForGame(pGame, asSkinNames);
	return !asSkinNames.empty();
}

std::string
NoteSkinManager::GetDefaultNoteSkinName()
{
	auto name = THEME->GetMetric("Common", "DefaultNoteSkinName");
	std::vector<std::string> all_names;
	GetAllNoteSkinNamesForGame(GAMESTATE->m_pCurGame, all_names);
	if (all_names.empty()) {
		return "";
	}
	if (!NoteSkinNameInList(name, all_names)) {
		name = "default";
		if (!NoteSkinNameInList(name, all_names)) {
			name = all_names[1];
		}
	}
	return name;
}

void
NoteSkinManager::ValidateNoteSkinName(std::string& name)
{
	if (name.empty() || !DoesNoteSkinExist(name)) {
		LuaHelpers::ReportScriptError(
		  "Someone set a noteskin that doesn't exist.  Good job.");
		name = GetDefaultNoteSkinName();
	}
}

std::string
NoteSkinManager::GetFirstWorkingNoteSkin()
{
	std::vector<std::string> all_names;
	GetAllNoteSkinNamesForGame(GAMESTATE->m_pCurGame, all_names);
	if (all_names.size() > 0)
		return all_names[0];
	return "";
}

void
NoteSkinManager::GetAllNoteSkinNamesForGame(const Game* pGame,
											std::vector<std::string>& AddTo)
{
	if (pGame == m_pCurGame) {
		// Faster:
		for (map<std::string, NoteSkinData>::const_iterator iter =
			   g_mapNameToData.begin();
			 iter != g_mapNameToData.end();
			 ++iter) {
			AddTo.push_back(iter->second.sName);
		}
	} else {
		std::string name = pGame->m_szName;
		if (name == "solo")
			name = "dance";
		auto sBaseSkinFolder = SpecialFiles::NOTESKINS_DIR + name + "/";
		GetDirListing(sBaseSkinFolder + "*", AddTo, true);
	}
}

std::string
NoteSkinManager::GetMetric(const std::string& sButtonName,
						   const std::string& sValue)
{
	if (m_sCurrentNoteSkin.empty()) {
		LuaHelpers::ReportScriptError(
		  "NOTESKIN:GetMetric: No noteskin currently set.", "NOTESKIN_ERROR");
		return "";
	}
	auto sNoteSkinName = make_lower(m_sCurrentNoteSkin);
	map<std::string, NoteSkinData>::const_iterator it =
	  g_mapNameToData.find(sNoteSkinName);
	ASSERT_M(it != g_mapNameToData.end(),
			 sNoteSkinName); // this NoteSkin doesn't exist!
	const auto& data = it->second;

	std::string sReturn;
	if (data.metrics.GetValue(sButtonName, sValue, sReturn))
		return sReturn;
	if (!data.metrics.GetValue("NoteDisplay", sValue, sReturn)) {
		LuaHelpers::ReportScriptError("Could not read metric \"" + sButtonName +
										"::" + sValue +
										"\" or \"NoteDisplay::" + sValue +
										"\" in \"" + sNoteSkinName + "\".",
									  "NOTESKIN_ERROR");
		return "";
	}
	return sReturn;
}

std::string
NoteSkinManager::GetMetric(const std::string& sButtonName,
						   const std::string& sValue, const std::string& sFallbackValue)
{
	if (m_sCurrentNoteSkin.empty()) {
		LuaHelpers::ReportScriptError(
		  "NOTESKIN:GetMetric: No noteskin currently set.", "NOTESKIN_ERROR");
		return "";
	}
	auto sNoteSkinName = make_lower(m_sCurrentNoteSkin);
	map<std::string, NoteSkinData>::const_iterator it =
	  g_mapNameToData.find(sNoteSkinName);
	ASSERT_M(it != g_mapNameToData.end(),
			 sNoteSkinName); // this NoteSkin doesn't exist!
	const auto& data = it->second;

	std::string sReturn;
	if (data.metrics.GetValue(sButtonName, sValue, sReturn))
		return sReturn;
	if (!data.metrics.GetValue("NoteDisplay", sValue, sReturn)) {
		// dont report an error
		return sFallbackValue;
	}
	return sReturn;
}

int
NoteSkinManager::GetMetricI(const std::string& sButtonName,
							const std::string& sValueName)
{
	return StringToInt(GetMetric(sButtonName, sValueName));
}

int
NoteSkinManager::GetMetricI(const std::string& sButtonName,
							const std::string& sValueName,
							const std::string& sDefaultValue)
{
	return StringToInt(GetMetric(sButtonName, sValueName, sDefaultValue));
}

float
NoteSkinManager::GetMetricF(const std::string& sButtonName,
							const std::string& sValueName)
{
	return StringToFloat(GetMetric(sButtonName, sValueName));
}

float
NoteSkinManager::GetMetricF(const std::string& sButtonName,
							const std::string& sValueName,
							const std::string& sDefaultValue)
{
	return StringToFloat(GetMetric(sButtonName, sValueName, sDefaultValue));
}

bool
NoteSkinManager::GetMetricB(const std::string& sButtonName,
							const std::string& sValueName)
{
	// Could also call GetMetricI here...hmm.
	return StringToInt(GetMetric(sButtonName, sValueName)) != 0;
}

bool
NoteSkinManager::GetMetricB(const std::string& sButtonName,
							const std::string& sValueName,
							const std::string& sDefaultValue)
{
	// Could also call GetMetricI here...hmm.
	return StringToInt(GetMetric(sButtonName, sValueName, sDefaultValue)) != 0;
}

apActorCommands
NoteSkinManager::GetMetricA(const std::string& sButtonName,
							const std::string& sValueName)
{
	return ActorUtil::ParseActorCommands(GetMetric(sButtonName, sValueName));
}

std::string
NoteSkinManager::GetPath(const std::string& sButtonName,
						 const std::string& sElement)
{
	const auto CacheString =
	  m_sCurrentNoteSkin + "/" + sButtonName + "/" + sElement;
	auto it = g_PathCache.find(CacheString);
	if (it != g_PathCache.end())
		return it->second;

	if (m_sCurrentNoteSkin.empty()) {
		LuaHelpers::ReportScriptError(
		  "NOTESKIN:GetPath: No noteskin currently set.", "NOTESKIN_ERROR");
		return "";
	}
	auto sNoteSkinName = make_lower(m_sCurrentNoteSkin);
	map<std::string, NoteSkinData>::const_iterator iter =
	  g_mapNameToData.find(sNoteSkinName);
	ASSERT(iter != g_mapNameToData.end());
	const auto& data = iter->second;

	std::string sPath; // fill this in below
	FOREACH_CONST(std::string, data.vsDirSearchOrder, lIter)
	{
		if (sButtonName.empty())
			sPath = GetPathFromDirAndFile(*lIter, sElement);
		else
			sPath = GetPathFromDirAndFile(*lIter, sButtonName + " " + sElement);
		if (!sPath.empty())
			break; // done searching
	}

	if (sPath.empty()) {
		FOREACH_CONST(std::string, data.vsDirSearchOrder, lIter)
		{
			if (!sButtonName.empty())
				sPath = GetPathFromDirAndFile(*lIter, "Fallback " + sElement);
			if (!sPath.empty())
				break; // done searching
		}
	}

	if (sPath.empty()) {
		std::string sPaths;
		FOREACH_CONST(std::string, data.vsDirSearchOrder, dir)
		{
			if (!sPaths.empty())
				sPaths += ", ";

			sPaths += *dir;
		}

		auto message =
		  ssprintf("The NoteSkin element \"%s %s\" could not be found in any "
				   "of the following directories:\n%s",
				   sButtonName.c_str(),
				   sElement.c_str(),
				   sPaths.c_str());

		switch (
		  LuaHelpers::ReportScriptError(message, "NOTESKIN_ERROR", true)) {
			case Dialog::retry:
				FOREACH_CONST(std::string, data.vsDirSearchOrder, dir)
				FILEMAN->FlushDirCache(*dir);
				g_PathCache.clear();
				return GetPath(sButtonName, sElement);
			case Dialog::abort:
				RageException::Throw("%s", message.c_str());
			case Dialog::ignore:
				return "";
			default:
				break;
		}
	}

	auto iLevel = 0;
	while (GetExtension(sPath) == "redir") {
		iLevel++;
		if (iLevel >= 100) {
			LuaHelpers::ReportScriptError(
			  "Infinite recursion while looking up " + sButtonName + " - " +
				sElement,
			  "NOTESKIN_ERROR");
			return "";
		}
		std::string sNewFileName;
		GetFileContents(sPath, sNewFileName, true);
		std::string sRealPath;

		FOREACH_CONST(std::string, data.vsDirSearchOrder, lIter)
		{
			sRealPath = GetPathFromDirAndFile(*lIter, sNewFileName);
			if (!sRealPath.empty())
				break; // done searching
		}

		if (sRealPath.empty()) {
			auto message =
			  ssprintf("NoteSkinManager:  The redirect \"%s\" points to the "
					   "file \"%s\", which does not exist. "
					   "Verify that this redirect is correct.",
					   sPath.c_str(),
					   sNewFileName.c_str());

			switch (
			  LuaHelpers::ReportScriptError(message, "NOTESKIN_ERROR", true)) {
				case Dialog::retry:
					FOREACH_CONST(std::string, data.vsDirSearchOrder, dir)
					FILEMAN->FlushDirCache(*dir);
					g_PathCache.clear();
					return GetPath(sButtonName, sElement);
				case Dialog::abort:
					RageException::Throw("%s", message.c_str());
				case Dialog::ignore:
					return "";
				default:
					break;
			}
		}

		sPath = sRealPath;
	}

	g_PathCache[CacheString] = sPath;
	return sPath;
}

bool
NoteSkinManager::PushActorTemplate(Lua* L,
								   const std::string& sButton,
								   const std::string& sElement,
								   bool bSpriteOnly,
								   std::string Color)
{
	map<std::string, NoteSkinData>::const_iterator iter =
	  g_mapNameToData.find(m_sCurrentNoteSkin);
	if (iter == g_mapNameToData.end()) {
		LuaHelpers::ReportScriptError("No current noteskin set!",
									  "NOTESKIN_ERROR");
		return false;
	}
	const auto& data = iter->second;

	LuaThreadVariable varPlayer("Player", LuaReference::Create(m_PlayerNumber));
	LuaThreadVariable varController("Controller",
									LuaReference::Create(m_GameController));
	LuaThreadVariable varButton("Button", sButton);
	LuaThreadVariable varElement("Element", sElement);
	LuaThreadVariable varSpriteOnly("SpriteOnly",
									LuaReference::Create(bSpriteOnly));
	LuaThreadVariable varColor("Color", Color);

	if (data.m_Loader.IsNil()) {
		LuaHelpers::ReportScriptError("No loader for noteskin!",
									  "NOTESKIN_ERROR");
		return false;
	}
	data.m_Loader.PushSelf(L);
	lua_remove(L, -2);
	lua_getfield(L, -1, "Load");

	return ActorUtil::LoadTableFromStackShowErrors(L);
}

Actor*
NoteSkinManager::LoadActor(const std::string& sButton,
						   const std::string& sElement,
						   Actor* pParent,
						   bool bSpriteOnly,
						   std::string Color)
{
	auto L = LUA->Get();

	if (!PushActorTemplate(L, sButton, sElement, bSpriteOnly, Color)) {
		LUA->Release(L);
		// ActorUtil will warn about the error
		return Sprite::NewBlankSprite();
	}

	std::unique_ptr<XNode> pNode(XmlFileUtil::XNodeFromTable(L));
	if (pNode.get() == nullptr) {
		LUA->Release(L);
		// XNode will warn about the error
		return Sprite::NewBlankSprite();
	}

	LUA->Release(L);

	auto pRet = ActorUtil::LoadFromNode(pNode.get(), pParent);

	if (bSpriteOnly) {
		// Make sure pActor is a Sprite (or something derived from Sprite).
		auto pSprite = dynamic_cast<Sprite*>(pRet);
		if (pSprite == nullptr) {
			LuaHelpers::ReportScriptErrorFmt("%s: %s %s must be a Sprite",
											 m_sCurrentNoteSkin.c_str(),
											 sButton.c_str(),
											 sElement.c_str());
			delete pRet;
			return Sprite::NewBlankSprite();
		}
	}

	return pRet;
}

std::string
NoteSkinManager::GetPathFromDirAndFile(const std::string& sDir,
									   const std::string& sFileName)
{
	std::vector<std::string> matches; // fill this with the possible files

	GetDirListing(sDir + sFileName + "*", matches, false, true);

	if (matches.empty())
		return std::string();

	if (matches.size() > 1) {
		auto sError = "Multiple files match '" + sDir + sFileName +
					  "'.  Please remove all but one of these files: ";
		sError += join(", ", matches);
		LuaHelpers::ReportScriptError(sError, "NOTESKIN_ERROR");
	}

	return matches[0];
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the NoteSkinManager. */
class LunaNoteSkinManager : public Luna<NoteSkinManager>
{
  public:
	DEFINE_METHOD(GetPath, GetPath(SArg(1), SArg(2)));
	DEFINE_METHOD(GetMetric, GetMetric(SArg(1), SArg(2)));
	DEFINE_METHOD(GetMetricI, GetMetricI(SArg(1), SArg(2)));
	DEFINE_METHOD(GetMetricF, GetMetricF(SArg(1), SArg(2)));
	DEFINE_METHOD(GetMetricB, GetMetricB(SArg(1), SArg(2)));
	static int GetMetricA(T* p, lua_State* L)
	{
		p->GetMetricA(SArg(1), SArg(2))->PushSelf(L);
		return 1;
	}
	static int LoadActor(T* p, lua_State* L)
	{
		std::string sButton = SArg(1);
		std::string sElement = SArg(2);
		if (!p->PushActorTemplate(L, sButton, sElement, false, "4th"))
			lua_pushnil(L);

		return 1;
	}
#define FOR_NOTESKIN(x, n)                                                     \
	static int x##ForNoteSkin(T* p, lua_State* L)                              \
	{                                                                          \
		const std::string sOldNoteSkin = p->GetCurrentNoteSkin();              \
		std::string nsname = SArg((n) + 1);                                    \
		if (!p->DoesNoteSkinExist(nsname)) {                                   \
			luaL_error(L, "Noteskin \"%s\" does not exist.", nsname.c_str());  \
		}                                                                      \
		p->SetCurrentNoteSkin(nsname);                                         \
		x(p, L);                                                               \
		p->SetCurrentNoteSkin(sOldNoteSkin);                                   \
		return 1;                                                              \
	}
	FOR_NOTESKIN(GetPath, 2);
	FOR_NOTESKIN(GetMetric, 2);
	FOR_NOTESKIN(GetMetricI, 2);
	FOR_NOTESKIN(GetMetricF, 2);
	FOR_NOTESKIN(GetMetricB, 2);
	FOR_NOTESKIN(GetMetricA, 2);
	FOR_NOTESKIN(LoadActor, 2);
#undef FOR_NOTESKIN
	static int AddChildActorForNoteSkin(T* p, lua_State* L) {
		const std::string sOldNoteSkin = p->GetCurrentNoteSkin();
		std::string nsname = SArg(3);
		if (!p->DoesNoteSkinExist(nsname)) {
			luaL_error(L, "Noteskin \"%s\" does not exist.", nsname.c_str());
		}

		auto actor = Luna<ActorFrame>::check(L, 4);

		p->SetCurrentNoteSkin(nsname);
		LoadActor(p, L);
		p->SetCurrentNoteSkin(sOldNoteSkin);

		auto xnode = XmlFileUtil::XNodeFromTable(L);
		if (xnode == nullptr) {
			// XNode will warn about the error
			lua_pushnil(L);
			return 1;
		}

		auto* result = ActorUtil::LoadFromNode(xnode, actor);
		if (result != nullptr) {
			actor->AddChild(result);
		}
		return 1;
	}

	static int GetNoteSkinNames(T* p, lua_State* L)
	{
		std::vector<std::string> vNoteskins;
		p->GetNoteSkinNames(vNoteskins);
		LuaHelpers::CreateTableFromArray(vNoteskins, L);
		return 1;
	}
	/*
	static int GetNoteSkinNamesForGame( T* p, lua_State *L )
	{
		Game *pGame = Luna<Game>::check( L, 1 );
		std::vector< std::string> vGameNoteskins;
		p->GetNoteSkinNames( pGame, vGameNoteskins );
		LuaHelpers::CreateTableFromArray(vGameNoteskins, L);
		return 1;
	}
	*/
	static int DoesNoteSkinExist(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->DoesNoteSkinExist(SArg(1)));
		return 1;
	}

	LunaNoteSkinManager()
	{
		ADD_METHOD(GetPath);
		ADD_METHOD(GetMetric);
		ADD_METHOD(GetMetricI);
		ADD_METHOD(GetMetricF);
		ADD_METHOD(GetMetricB);
		ADD_METHOD(GetMetricA);
		ADD_METHOD(LoadActor);
		ADD_METHOD(GetPathForNoteSkin);
		ADD_METHOD(GetMetricForNoteSkin);
		ADD_METHOD(GetMetricIForNoteSkin);
		ADD_METHOD(GetMetricFForNoteSkin);
		ADD_METHOD(GetMetricBForNoteSkin);
		ADD_METHOD(GetMetricAForNoteSkin);
		ADD_METHOD(LoadActorForNoteSkin);
		ADD_METHOD(AddChildActorForNoteSkin);
		ADD_METHOD(GetNoteSkinNames);
		ADD_METHOD(DoesNoteSkinExist); // for the current game
	}
};

LUA_REGISTER_CLASS(NoteSkinManager)
// lua end
