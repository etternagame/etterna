#include "Etterna/Globals/global.h"
#include "AnnouncerManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include <cstring>

AnnouncerManager* ANNOUNCER =
  nullptr; // global and accessible from anywhere in our program

const std::string EMPTY_ANNOUNCER_NAME = "Empty";
const std::string ANNOUNCERS_DIR = "Announcers/";

AnnouncerManager::AnnouncerManager()
{
	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "ANNOUNCER");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

AnnouncerManager::~AnnouncerManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("ANNOUNCER");
}

void
AnnouncerManager::GetAnnouncerNames(std::vector<std::string>& AddTo)
{
	FILEMAN->GetDirListing(ANNOUNCERS_DIR + "*", AddTo, ONLY_DIR);

	// strip out the empty announcer folder
	for (int i = AddTo.size() - 1; i >= 0; i--)
		if (!strcasecmp(AddTo[i].c_str(), EMPTY_ANNOUNCER_NAME.c_str()))
			AddTo.erase(AddTo.begin() + i, AddTo.begin() + i + 1);
}

bool
AnnouncerManager::DoesAnnouncerExist(const std::string& sAnnouncerName)
{
	if (sAnnouncerName.empty())
		return true;

	std::vector<std::string> asAnnouncerNames;
	GetAnnouncerNames(asAnnouncerNames);
	for (unsigned i = 0; i < asAnnouncerNames.size(); i++)
		if (0 ==
			strcasecmp(sAnnouncerName.c_str(), asAnnouncerNames[i].c_str()))
			return true;
	return false;
}

std::string
AnnouncerManager::GetAnnouncerDirFromName(const std::string& sAnnouncerName)
{
	return ANNOUNCERS_DIR + sAnnouncerName + "/";
}

void
AnnouncerManager::SwitchAnnouncer(const std::string& sNewAnnouncerName)
{
	if (!DoesAnnouncerExist(sNewAnnouncerName))
		m_sCurAnnouncerName = "";
	else
		m_sCurAnnouncerName = sNewAnnouncerName;
}

/* Aliases for announcer paths.  This is for compatibility, so we don't force
 * announcer changes along with everything else.  We could use it to support
 * DWI announcers transparently, too. */
static const char* aliases[][2] = {
	/* ScreenSelectDifficulty compatibility: */
	{ "ScreenSelectDifficulty comment beginner",
	  "select difficulty comment beginner" },
	{ "ScreenSelectDifficulty comment easy", "select difficulty comment easy" },
	{ "ScreenSelectDifficulty comment medium",
	  "select difficulty comment medium" },
	{ "ScreenSelectDifficulty comment hard", "select difficulty comment hard" },
	{ "ScreenSelectDifficulty intro", "select difficulty intro" },

	/* ScreenSelectStyle compatibility: */
	{ "ScreenSelectStyle intro", "select style intro" },
	{ "ScreenSelectStyle comment single", "select style comment single" },
	{ "ScreenSelectStyle comment double", "select style comment double" },
	{ "ScreenSelectStyle comment solo", "select style comment solo" },
	{ "ScreenSelectStyle comment versus", "select style comment versus" },

	/* Combo compatibility: */
	{ "gameplay combo 100", "gameplay 100 combo" },
	{ "gameplay combo 200", "gameplay 200 combo" },
	{ "gameplay combo 300", "gameplay 300 combo" },
	{ "gameplay combo 400", "gameplay 400 combo" },
	{ "gameplay combo 500", "gameplay 500 combo" },
	{ "gameplay combo 600", "gameplay 600 combo" },
	{ "gameplay combo 700", "gameplay 700 combo" },
	{ "gameplay combo 800", "gameplay 800 combo" },
	{ "gameplay combo 900", "gameplay 900 combo" },
	{ "gameplay combo 1000", "gameplay 1000 combo" },

	{ nullptr, nullptr }
};

/* Find an announcer directory with sounds in it.  First search sFolderName,
 * then all aliases above.  Ignore directories that are empty, since we might
 * have "select difficulty intro" with sounds and an empty
 * "ScreenSelectDifficulty intro". */
std::string
AnnouncerManager::GetPathTo(const std::string& sAnnouncerName,
							const std::string& sFolderName)
{
	if (sAnnouncerName.empty())
		return std::string(); /* announcer disabled */

	const std::string AnnouncerPath = GetAnnouncerDirFromName(sAnnouncerName);

	if (!DirectoryIsEmpty(AnnouncerPath + sFolderName + "/"))
		return AnnouncerPath + sFolderName + "/";

	/* Search for the announcer folder in the list of aliases. */
	int i;
	for (i = 0; aliases[i][0] != nullptr; ++i) {
		if (!EqualsNoCase(sFolderName, aliases[i][0]))
			continue; /* no match */

		if (!DirectoryIsEmpty(AnnouncerPath + aliases[i][1] + "/"))
			return AnnouncerPath + aliases[i][1] + "/";
	}

	/* No announcer directory matched.  In debug, create the directory by
	 * its preferred name. */
#ifdef DEBUG
	Locator::getLogger()->warn("The announcer in '{}' is missing the folder '{}'.",
			   AnnouncerPath.c_str(), sFolderName.c_str());
	//	MessageBeep( MB_OK );
	RageFile temp;
	temp.Open(AnnouncerPath + sFolderName + "/announcer files go here.txt",
			  RageFile::WRITE);
#endif

	return std::string();
}

std::string
AnnouncerManager::GetPathTo(const std::string& sFolderName)
{
	return GetPathTo(m_sCurAnnouncerName, sFolderName);
}

bool
AnnouncerManager::HasSoundsFor(const std::string& sFolderName)
{
	return !DirectoryIsEmpty(GetPathTo(sFolderName));
}

void
AnnouncerManager::NextAnnouncer()
{
	std::vector<std::string> as;
	GetAnnouncerNames(as);
	if (as.empty())
		return;

	if (m_sCurAnnouncerName.empty())
		SwitchAnnouncer(as[0]);
	else {
		unsigned i;
		for (i = 0; i < as.size(); i++)
			if (EqualsNoCase(as[i], m_sCurAnnouncerName))
				break;
		if (i == as.size() - 1)
			SwitchAnnouncer("");
		else {
			int iNewIndex = (i + 1) % as.size();
			SwitchAnnouncer(as[iNewIndex]);
		}
	}
}
// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the AnnouncerManager. */
class LunaAnnouncerManager : public Luna<AnnouncerManager>
{
  public:
	static int DoesAnnouncerExist(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->DoesAnnouncerExist(SArg(1)));
		return 1;
	}
	static int GetAnnouncerNames(T* p, lua_State* L)
	{
		std::vector<std::string> vAnnouncers;
		p->GetAnnouncerNames(vAnnouncers);
		LuaHelpers::CreateTableFromArray(vAnnouncers, L);
		return 1;
	}
	static int GetCurrentAnnouncer(T* p, lua_State* L)
	{
		std::string s = p->GetCurAnnouncerName();
		if (s.empty()) {
			lua_pushnil(L);
		} else {
			lua_pushstring(L, s.c_str());
		}
		return 1;
	}
	static int SetCurrentAnnouncer(T* p, lua_State* L)
	{
		std::string s = SArg(1);
		// only bother switching if the announcer exists. -aj
		if (p->DoesAnnouncerExist(s))
			p->SwitchAnnouncer(s);
		COMMON_RETURN_SELF;
	}

	LunaAnnouncerManager()
	{
		ADD_METHOD(DoesAnnouncerExist);
		ADD_METHOD(GetAnnouncerNames);
		ADD_METHOD(GetCurrentAnnouncer);
		ADD_METHOD(SetCurrentAnnouncer);
	}
};

LUA_REGISTER_CLASS(AnnouncerManager)
// lua end
