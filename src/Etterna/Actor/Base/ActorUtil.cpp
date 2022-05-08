#include "Etterna/Globals/global.h"
#include "ActorUtil.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "arch/Dialog/Dialog.h"

#include <map>
#include <algorithm>

using std::map;

// Actor registration
static map<std::string, CreateActorFn>* g_pmapRegistrees = nullptr;

static bool
IsRegistered(const std::string& sClassName)
{
	return g_pmapRegistrees->find(sClassName) != g_pmapRegistrees->end();
}

void
ActorUtil::Register(const std::string& sClassName, CreateActorFn pfn)
{
	if (g_pmapRegistrees == nullptr)
		g_pmapRegistrees = new map<std::string, CreateActorFn>;

	const auto iter = g_pmapRegistrees->find(sClassName);
	ASSERT_M(
	  iter == g_pmapRegistrees->end(),
	  ssprintf("Actor class '%s' already registered.", sClassName.c_str()));

	(*g_pmapRegistrees)[sClassName] = pfn;
}

/* Resolves actor paths a la LoadActor("..."), with autowildcarding and .redir
 * files.  Returns a path *within* the Rage filesystem, unlike the FILEMAN
 * function of the same name. */
bool
ActorUtil::ResolvePath(std::string& sPath,
					   const std::string& sName,
					   bool optional)
{
	CollapsePath(sPath);

	// If we know this is an exact match, don't bother with the GetDirListing,
	// so "foo" doesn't partial match "foobar" if "foo" exists.
	const auto ft = FILEMAN->GetFileType(sPath);
	if (ft != RageFileManager::TYPE_FILE && ft != RageFileManager::TYPE_DIR) {
		std::vector<std::string> asPaths;
		GetDirListing(sPath + "*", asPaths, false, true); // return path too

		if (asPaths.empty()) {
			if (optional) {
				return false;
			}
			const auto sError =
			  ssprintf("%s: references a file \"%s\" which doesn't exist",
					   sName.c_str(),
					   sPath.c_str());
			switch (LuaHelpers::ReportScriptError(
			  sError, "BROKEN_FILE_REFERENCE", true)) {
				case Dialog::abort:
					RageException::Throw("%s", sError.c_str());
				case Dialog::retry:
					FILEMAN->FlushDirCache();
					return ResolvePath(sPath, sName);
				case Dialog::ignore:
					return false;
				default:
					FAIL_M("Invalid response to Abort/Retry/Ignore dialog");
			}
		}

		THEME->FilterFileLanguages(asPaths);

		if (asPaths.size() > 1) {
			auto sError = ssprintf(
			  "%s: references a file \"%s\" which has multiple matches",
			  sName.c_str(),
			  sPath.c_str());
			sError += "\n" + join("\n", asPaths);
			switch (LuaHelpers::ReportScriptError(
			  sError, "BROKEN_FILE_REFERENCE", true)) {
				case Dialog::abort:
					RageException::Throw("%s", sError.c_str());
				case Dialog::retry:
					FILEMAN->FlushDirCache();
					return ResolvePath(sPath, sName);
				case Dialog::ignore:
					asPaths.erase(asPaths.begin() + 1, asPaths.end());
					break;
				default:
					FAIL_M("Invalid response to Abort/Retry/Ignore dialog");
			}
		}

		sPath = asPaths[0];
	}

	if (ft == RageFileManager::TYPE_DIR) {
		const auto sLuaPath = sPath + "/default.lua";
		if (DoesFileExist(sLuaPath)) {
			sPath = sLuaPath;
			return true;
		}
	}

	sPath = DerefRedir(sPath);
	return true;
}

namespace {
std::string
GetLegacyActorClass(XNode* pActor)
{
	ASSERT(pActor != nullptr);

	// The non-legacy LoadFromNode has already checked the Class and
	// Type attributes.

	if (pActor->GetAttr("Text") != nullptr)
		return "BitmapText";

	std::string sFile;
	if (pActor->GetAttrValue("File", sFile) && !sFile.empty()) {
		// Backward compatibility hacks for "special" filenames
		if (EqualsNoCase(sFile, "songbackground")) {
			auto* pVal = new XNodeStringValue;
			Song* pSong = GAMESTATE->m_pCurSong;
			if (pSong && pSong->HasBackground())
				pVal->SetValue(pSong->GetBackgroundPath());
			else
				pVal->SetValue(
				  THEME->GetPathG("Common", "fallback background"));
			pActor->AppendAttrFrom("Texture", pVal, false);
			return "Sprite";
		}
		if (EqualsNoCase(sFile, "songbanner")) {
			auto* pVal = new XNodeStringValue;
			Song* pSong = GAMESTATE->m_pCurSong;
			if (pSong && pSong->HasBanner())
				pVal->SetValue(pSong->GetBannerPath());
			else
				pVal->SetValue(THEME->GetPathG("Common", "fallback banner"));
			pActor->AppendAttrFrom("Texture", pVal, false);
			return "Sprite";
		}
	}

	// Fallback: use XML tag name for actor class
	return pActor->m_sName;
}
} // namespace

Actor*
ActorUtil::LoadFromNode(const XNode* _pNode, Actor* pParentActor)
{
	ASSERT(_pNode != nullptr);

	auto node = *_pNode;

	// Remove this in favor of using conditionals in Lua. -Chris
	// There are a number of themes out there that depend on this (including
	// sm-ssc default). Probably for the best to leave this in. -aj
	{
		bool bCond;
		if (node.GetAttrValue("Condition", bCond) && !bCond)
			return nullptr;
	}

	std::string sClass;
	auto bHasClass = node.GetAttrValue("Class", sClass);
	if (!bHasClass)
		bHasClass = node.GetAttrValue("Type", sClass);

	const auto bLegacy = (node.GetAttr("_LegacyXml") != nullptr);
	if (!bHasClass && bLegacy)
		sClass = GetLegacyActorClass(&node);

	const auto iter = g_pmapRegistrees->find(sClass);
	if (iter == g_pmapRegistrees->end()) {
		std::string sFile;
		if (bLegacy && node.GetAttrValue("File", sFile) && !sFile.empty()) {
			std::string sPath;
			// Handle absolute paths correctly
			if (sFile.front() == '/')
				sPath = sFile;
			else
				sPath = Dirname(GetSourcePath(&node)) + sFile;
			if (ResolvePath(sPath, GetWhere(&node))) {
				auto pNewActor = MakeActor(sPath, pParentActor);
				if (pNewActor == nullptr)
					return nullptr;
				if (pParentActor != nullptr)
					pNewActor->SetParent(pParentActor);
				pNewActor->LoadFromNode(&node);
				return pNewActor;
			}
		}

		// sClass is invalid
		const auto sError = ssprintf("%s: invalid Class \"%s\"",
									 ActorUtil::GetWhere(&node).c_str(),
									 sClass.c_str());
		LuaHelpers::ReportScriptError(sError);
		return new Actor; // Return a dummy object so that we don't crash in
						  // AutoActor later.
	}

	const auto& pfn = iter->second;
	auto pRet = pfn();

	if (pParentActor != nullptr)
		pRet->SetParent(pParentActor);

	pRet->LoadFromNode(&node);
	return pRet;
}

namespace {
XNode*
LoadXNodeFromLuaShowErrors(const std::string& sFile)
{
	std::string sScript;
	if (!GetFileContents(sFile, sScript))
		return nullptr;

	auto L = LUA->Get();

	std::string sError;
	if (!LuaHelpers::LoadScript(L, sScript, "@" + sFile, sError)) {
		LUA->Release(L);
		sError = ssprintf("Lua runtime error: %s", sError.c_str());
		LuaHelpers::ReportScriptError(sError);
		return nullptr;
	}

	XNode* pRet = nullptr;
	if (ActorUtil::LoadTableFromStackShowErrors(L))
		pRet = XmlFileUtil::XNodeFromTable(L);

	LUA->Release(L);
	return pRet;
}
} // namespace

/* Run the function at the top of the stack, which returns an actor description
 * table.  If the table was returned, return true and leave it on the stack.
 * If not, display an error, return false, and leave nothing on the stack. */
bool
ActorUtil::LoadTableFromStackShowErrors(Lua* L)
{
	LuaReference func;
	lua_pushvalue(L, -1);
	func.SetFromStack(L);

	std::string Error = "Lua runtime error: ";
	if (!LuaHelpers::RunScriptOnStack(L, Error, 0, 1, true)) {
		lua_pop(L, 1);
		return false;
	}

	if (lua_type(L, -1) != LUA_TTABLE) {
		lua_pop(L, 1);

		func.PushSelf(L);
		lua_Debug debug;
		lua_getinfo(L, ">nS", &debug);

		Error = ssprintf("%s: must return a table", debug.short_src);

		LuaHelpers::ReportScriptError(Error, "LUA_ERROR");
		return false;
	}
	return true;
}

// NOTE: This function can return NULL if the actor should not be displayed.
// Callers should be aware of this and handle it appropriately.
Actor*
ActorUtil::MakeActor(const std::string& sPath_, Actor* pParentActor)
{
	auto sPath(sPath_);

	auto ft = GetFileType(sPath);
	switch (ft) {
		case FT_Lua: {
			std::unique_ptr<XNode> pNode(LoadXNodeFromLuaShowErrors(sPath));
			if (pNode.get() == nullptr) {
				// XNode will warn about the error
				return new Actor;
			}

			auto pRet = ActorUtil::LoadFromNode(pNode.get(), pParentActor);
			return pRet;
		}
		case FT_Xml: {
			return new Actor;
		}
		case FT_Directory: {
			if (sPath.back() != '/')
				sPath += '/';

			auto sXml = sPath + "default.xml";
			if (DoesFileExist(sXml))
				return MakeActor(sXml, pParentActor);

			XNode xml;
			xml.AppendAttr("Class", "BGAnimation");
			xml.AppendAttr("AniDir", sPath);

			return ActorUtil::LoadFromNode(&xml, pParentActor);
		}
		case FT_Bitmap:
		case FT_Movie: {
			XNode xml;
			xml.AppendAttr("Class", "Sprite");
			xml.AppendAttr("Texture", sPath);

			return ActorUtil::LoadFromNode(&xml, pParentActor);
		}
		case FT_Sprite: {
			return new Actor;
			case FT_Model: {
				XNode xml;
				xml.AppendAttr("Class", "Model");
				xml.AppendAttr("Meshes", sPath);
				xml.AppendAttr("Materials", sPath);
				xml.AppendAttr("Bones", sPath);

				return ActorUtil::LoadFromNode(&xml, pParentActor);
			}
			default: {
				Locator::getLogger()->warn("File \"{}\" has unknown type, \"{}\".",
						  sPath.c_str(), FileTypeToString(ft).c_str());

				XNode xml;
				xml.AppendAttr("Class", "Actor");
				return ActorUtil::LoadFromNode(&xml, pParentActor);
			}
		}
	}
}

std::string
ActorUtil::GetSourcePath(const XNode* pNode)
{
	std::string sRet;
	pNode->GetAttrValue("_Source", sRet);
	if (sRet.substr(0, 1) == "@")
		sRet.erase(0, 1);

	return sRet;
}

std::string
ActorUtil::GetWhere(const XNode* pNode)
{
	auto sPath = GetSourcePath(pNode);

	int iLine;
	if (pNode->GetAttrValue("_Line", iLine))
		sPath += ssprintf(":%i", iLine);
	return sPath;
}

bool
ActorUtil::GetAttrPath(const XNode* pNode,
					   const std::string& sName,
					   std::string& sOut,
					   bool optional)
{
	if (!pNode->GetAttrValue(sName, sOut))
		return false;

	const auto bIsRelativePath = sOut.front() != '/';
	if (bIsRelativePath) {
		std::string sDir;
		if (!pNode->GetAttrValue("_Dir", sDir)) {
			if (!optional) {
				Locator::getLogger()->warn("Relative path \"{}\", but path is unknown", sOut.c_str());
			}
			return false;
		}
		sOut = sDir + sOut;
	}

	return ActorUtil::ResolvePath(sOut, ActorUtil::GetWhere(pNode), optional);
}

apActorCommands
ActorUtil::ParseActorCommands(const std::string& sCommands,
							  const std::string& sName)
{
	auto L = LUA->Get();
	LuaHelpers::ParseCommandList(L, sCommands, sName, false);
	auto* pRet = new LuaReference;
	pRet->SetFromStack(L);
	LUA->Release(L);

	return apActorCommands(pRet);
}

void
ActorUtil::SetXY(Actor& actor, const std::string& sMetricsGroup)
{
	ASSERT(!actor.GetName().empty());

	/*
	 * Hack: This is run after InitCommand, and InitCommand might set X/Y.  If
	 * these are both 0, leave the actor where it is.  If InitCommand doesn't,
	 * then 0,0 is the default, anyway.
	 */
	const auto fX = THEME->GetMetricF(sMetricsGroup, actor.GetName() + "X");
	const auto fY = THEME->GetMetricF(sMetricsGroup, actor.GetName() + "Y");
	if (fX != 0 || fY != 0)
		actor.SetXY(fX, fY);
}

void
ActorUtil::LoadCommand(Actor& actor,
					   const std::string& sMetricsGroup,
					   const std::string& sCommandName)
{
	ActorUtil::LoadCommandFromName(
	  actor, sMetricsGroup, sCommandName, actor.GetName());
}

void
ActorUtil::LoadCommandFromName(Actor& actor,
							   const std::string& sMetricsGroup,
							   const std::string& sCommandName,
							   const std::string& sName)
{
	actor.AddCommand(
	  sCommandName,
	  THEME->GetMetricA(sMetricsGroup, sName + sCommandName + "Command"));
}

void
ActorUtil::LoadAllCommands(Actor& actor, const std::string& sMetricsGroup)
{
	LoadAllCommandsFromName(actor, sMetricsGroup, actor.GetName());
}

void
ActorUtil::LoadAllCommandsFromName(Actor& actor,
								   const std::string& sMetricsGroup,
								   const std::string& sName)
{
	std::set<std::string> vsValueNames;
	THEME->GetMetricsThatBeginWith(sMetricsGroup, sName, vsValueNames);

	for (const auto& s : vsValueNames) {
		static const std::string sEnding = "Command";
		if (EndsWith(s, sEnding)) {
			std::string sCommandName(s.begin() + sName.size(),
									 s.end() - sEnding.size());
			LoadCommandFromName(actor, sMetricsGroup, sCommandName, sName);
		}
	}
}

static bool
CompareActorsByZPosition(const Actor* p1, const Actor* p2)
{
	return p1->GetZ() < p2->GetZ();
}

void
ActorUtil::SortByZPosition(std::vector<Actor*>& vActors)
{
	// Preserve ordering of Actors with equal Z positions.
	stable_sort(vActors.begin(), vActors.end(), CompareActorsByZPosition);
}

static const char* FileTypeNames[] = {
	"Bitmap", "Sprite", "Sound", "Movie", "Directory",
	"Xml",	  "Model",	"Lua",	 "Ini",
};
XToString(FileType);

LuaXType(FileType);

// convenience so the for-loop lines can be shorter.
typedef map<std::string, FileType> etft_cont_t;
typedef map<FileType, std::vector<std::string>> fttel_cont_t;
etft_cont_t ExtensionToFileType;
fttel_cont_t FileTypeToExtensionList;

void
ActorUtil::InitFileTypeLists()
{
	// This function creates things to serve two purposes:
	// 1.  A map from extensions to filetypes, so extensions can be converted.
	// 2.  A reverse map for things that need a list of extensions to look for.
	// The first section creates the map from extensions to filetypes, then the
	// second section uses that map to build the reverse map.
	ExtensionToFileType["lua"] = FT_Lua;

	ExtensionToFileType["xml"] = FT_Xml;

	ExtensionToFileType["ini"] = FT_Ini;

	// Update RageSurfaceUtils when adding new image formats.
	ExtensionToFileType["bmp"] = FT_Bitmap;
	ExtensionToFileType["gif"] = FT_Bitmap;
	ExtensionToFileType["jpeg"] = FT_Bitmap;
	ExtensionToFileType["jpg"] = FT_Bitmap;
	ExtensionToFileType["png"] = FT_Bitmap;

	// Update RageSoundReader_FileReader when adding new sound formats.
	ExtensionToFileType["mp3"] = FT_Sound;
	ExtensionToFileType["oga"] = FT_Sound;
	ExtensionToFileType["ogg"] = FT_Sound;
	ExtensionToFileType["wav"] = FT_Sound;

	// ffmpeg takes care of loading videos, not sure whether this list should
	// have everything ffmpeg supports.
	ExtensionToFileType["avi"] = FT_Movie;
	ExtensionToFileType["f4v"] = FT_Movie;
	ExtensionToFileType["flv"] = FT_Movie;
	ExtensionToFileType["mkv"] = FT_Movie;
	ExtensionToFileType["mp4"] = FT_Movie;
	ExtensionToFileType["mpeg"] = FT_Movie;
	ExtensionToFileType["mpg"] = FT_Movie;
	ExtensionToFileType["mov"] = FT_Movie;
	ExtensionToFileType["ogv"] = FT_Movie;
	ExtensionToFileType["webm"] = FT_Movie;
	ExtensionToFileType["wmv"] = FT_Movie;

	ExtensionToFileType["sprite"] = FT_Sprite;

	ExtensionToFileType["txt"] = FT_Model;

	// When adding new extensions, do not add them below this line.  This line
	// marks the point where the function switches to building the reverse map.
	for (auto& curr_ext : ExtensionToFileType) {
		FileTypeToExtensionList[curr_ext.second].push_back(curr_ext.first);
	}
}

std::vector<std::string> const&
ActorUtil::GetTypeExtensionList(FileType ft)
{
	return FileTypeToExtensionList[ft];
}

void
ActorUtil::AddTypeExtensionsToList(FileType ft, std::vector<std::string>& add_to)
{
	auto ext_list = FileTypeToExtensionList.find(ft);
	if (ext_list != FileTypeToExtensionList.end()) {
		add_to.reserve(add_to.size() + ext_list->second.size());
		for (auto& curr : ext_list->second) {
			add_to.push_back(curr);
		}
	}
}

FileType
ActorUtil::GetFileType(const std::string& sPath)
{
	const auto sExt = make_lower(GetExtension(sPath));

	const auto conversion_entry = ExtensionToFileType.find(sExt);
	if (conversion_entry != ExtensionToFileType.end()) {
		return conversion_entry->second;
	}
	if (!sPath.empty() && sPath[sPath.size() - 1] == '/') {
		return FT_Directory;
	}
	/* Do this last, to avoid the IsADirectory in most cases. */
	if (IsADirectory(sPath)) {
		return FT_Directory;
	}
	return FileType_Invalid;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

namespace {
int
GetFileType(lua_State* L)
{
	Enum::Push(L, ActorUtil::GetFileType(SArg(1)));
	return 1;
}

int
ResolvePath(lua_State* L)
{
	std::string sPath(SArg(1));
	const auto iLevel = IArg(2);
	const auto optional = lua_toboolean(L, 3) != 0;
	luaL_where(L, iLevel);
	std::string sWhere = lua_tostring(L, -1);
	if (sWhere.size() > 2 && sWhere.substr(sWhere.size() - 2, 2) == ": ")
		sWhere = sWhere.substr(0, sWhere.size() - 2); // remove trailing ": "

	LUA->YieldLua();
	const auto bRet = ActorUtil::ResolvePath(sPath, sWhere, optional);
	LUA->UnyieldLua();

	if (!bRet)
		return 0;
	LuaHelpers::Push(L, sPath);
	return 1;
}
int
IsRegisteredClass(lua_State* L)
{
	lua_pushboolean(L, IsRegistered(SArg(1)));
	return 1;
}
static void
name_error(Actor* p, lua_State* L)
{
	if (p->GetName().empty()) {
		luaL_error(L, "LoadAllCommands requires the actor to have a name.");
	}
}
static int
LoadAllCommands(lua_State* L)
{
	auto p = Luna<Actor>::check(L, 1);
	name_error(p, L);
	ActorUtil::LoadAllCommands(p, SArg(2));
	return 0;
}
static int
LoadAllCommandsFromName(lua_State* L)
{
	auto p = Luna<Actor>::check(L, 1);
	name_error(p, L);
	ActorUtil::LoadAllCommandsFromName(*p, SArg(2), SArg(3));
	return 0;
}
static int
LoadAllCommandsAndSetXY(lua_State* L)
{
	auto p = Luna<Actor>::check(L, 1);
	name_error(p, L);
	ActorUtil::LoadAllCommandsAndSetXY(p, SArg(2));
	return 0;
}

const luaL_Reg ActorUtilTable[] = { LIST_METHOD(GetFileType),
									LIST_METHOD(ResolvePath),
									LIST_METHOD(IsRegisteredClass),
									LIST_METHOD(LoadAllCommands),
									LIST_METHOD(LoadAllCommandsFromName),
									LIST_METHOD(LoadAllCommandsAndSetXY),
									{ nullptr, nullptr } };
} // namespace

LUA_REGISTER_NAMESPACE(ActorUtil)
