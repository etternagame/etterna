#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Character.h"
#include "Etterna/FileTypes/IniFile.h"
#include "RageUtil/Graphics/RageTextureID.h"
#include "RageUtil/Utils/RageUtil.h"

RString
GetRandomFileInDir(const RString& sDir);

Character::Character()
  : m_sCharDir("")
  , m_sCharacterID("")
  , m_sDisplayName("")
  , m_sCardPath("")
  , m_sIconPath("")
{
}

bool
Character::Load(RString sCharDir)
{
	// Save character directory
	if (sCharDir.Right(1) != "/")
		sCharDir += "/";
	m_sCharDir = sCharDir;

	// save ID
	{
		vector<RString> as;
		split(sCharDir, "/", as);
		m_sCharacterID = as.back();
	}

	{
		vector<RString> as;
		GetDirListing(m_sCharDir + "card.png", as, false, true);
		GetDirListing(m_sCharDir + "card.jpg", as, false, true);
		GetDirListing(m_sCharDir + "card.jpeg", as, false, true);
		GetDirListing(m_sCharDir + "card.gif", as, false, true);
		GetDirListing(m_sCharDir + "card.bmp", as, false, true);
		if (as.empty())
			m_sCardPath = "";
		else
			m_sCardPath = as[0];
	}

	{
		vector<RString> as;
		GetDirListing(m_sCharDir + "icon.png", as, false, true);
		GetDirListing(m_sCharDir + "icon.jpg", as, false, true);
		GetDirListing(m_sCharDir + "icon.jpeg", as, false, true);
		GetDirListing(m_sCharDir + "icon.gif", as, false, true);
		GetDirListing(m_sCharDir + "icon.bmp", as, false, true);
		if (as.empty())
			m_sIconPath = "";
		else
			m_sIconPath = as[0];
	}

	// Save attacks
	IniFile ini;
	if (!ini.ReadFile(sCharDir + "character.ini"))
		return false;

	// get optional display name
	ini.GetValue("Character", "DisplayName", m_sDisplayName);

	// get optional InitCommand
	RString s;
	ini.GetValue("Character", "InitCommand", s);
	m_cmdInit = ActorUtil::ParseActorCommands(s);

	return true;
}

RString
GetRandomFileInDir(const RString& sDir)
{
	vector<RString> asFiles;
	GetDirListing(sDir, asFiles, false, true);
	if (asFiles.empty())
		return RString();
	else
		return asFiles[RandomInt(asFiles.size())];
}

RString
Character::GetModelPath() const
{
	RString s = m_sCharDir + "model.txt";
	if (DoesFileExist(s))
		return s;
	else
		return RString();
}

RString
Character::GetRestAnimationPath() const
{
	return DerefRedir(GetRandomFileInDir(m_sCharDir + "Rest/"));
}
RString
Character::GetWarmUpAnimationPath() const
{
	return DerefRedir(GetRandomFileInDir(m_sCharDir + "WarmUp/"));
}
RString
Character::GetDanceAnimationPath() const
{
	return DerefRedir(GetRandomFileInDir(m_sCharDir + "Dance/"));
}
RString
Character::GetTakingABreakPath() const
{
	vector<RString> as;
	GetDirListing(m_sCharDir + "break.png", as, false, true);
	GetDirListing(m_sCharDir + "break.jpg", as, false, true);
	GetDirListing(m_sCharDir + "break.jpeg", as, false, true);
	GetDirListing(m_sCharDir + "break.gif", as, false, true);
	GetDirListing(m_sCharDir + "break.bmp", as, false, true);
	if (as.empty())
		return RString();
	else
		return as[0];
}

RString
Character::GetSongSelectIconPath() const
{
	vector<RString> as;
	// first try and find an icon specific to the select music screen
	// so you can have different icons for music select / char select
	GetDirListing(m_sCharDir + "selectmusicicon.png", as, false, true);
	GetDirListing(m_sCharDir + "selectmusicicon.jpg", as, false, true);
	GetDirListing(m_sCharDir + "selectmusicicon.jpeg", as, false, true);
	GetDirListing(m_sCharDir + "selectmusicicon.gif", as, false, true);
	GetDirListing(m_sCharDir + "selectmusicicon.bmp", as, false, true);

	if (as.empty()) {
		// if that failed, try using the regular icon
		GetDirListing(m_sCharDir + "icon.png", as, false, true);
		GetDirListing(m_sCharDir + "icon.jpg", as, false, true);
		GetDirListing(m_sCharDir + "icon.jpeg", as, false, true);
		GetDirListing(m_sCharDir + "icon.gif", as, false, true);
		GetDirListing(m_sCharDir + "icon.bmp", as, false, true);
		if (as.empty())
			return RString();
		else
			return as[0];
	} else
		return as[0];
}

RString
Character::GetStageIconPath() const
{
	vector<RString> as;
	// first try and find an icon specific to the select music screen
	// so you can have different icons for music select / char select
	GetDirListing(m_sCharDir + "stageicon.png", as, false, true);
	GetDirListing(m_sCharDir + "stageicon.jpg", as, false, true);
	GetDirListing(m_sCharDir + "stageicon.jpeg", as, false, true);
	GetDirListing(m_sCharDir + "stageicon.gif", as, false, true);
	GetDirListing(m_sCharDir + "stageicon.bmp", as, false, true);

	if (as.empty()) {
		// if that failed, try using the regular icon
		GetDirListing(m_sCharDir + "card.png", as, false, true);
		GetDirListing(m_sCharDir + "card.jpg", as, false, true);
		GetDirListing(m_sCharDir + "card.jpeg", as, false, true);
		GetDirListing(m_sCharDir + "card.gif", as, false, true);
		GetDirListing(m_sCharDir + "card.bmp", as, false, true);
		if (as.empty())
			return RString();
		else
			return as[0];
	} else
		return as[0];
}

bool
Character::Has2DElems()
{
	if (DoesFileExist(m_sCharDir +
					  "2DFail/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	if (DoesFileExist(m_sCharDir +
					  "2DFever/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	if (DoesFileExist(m_sCharDir +
					  "2DGood/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	if (DoesFileExist(m_sCharDir +
					  "2DMiss/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	if (DoesFileExist(m_sCharDir +
					  "2DWin/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	if (DoesFileExist(
		  m_sCharDir +
		  "2DWinFever/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	if (DoesFileExist(m_sCharDir +
					  "2DGreat/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	if (DoesFileExist(m_sCharDir +
					  "2DIdle/BGAnimation.ini")) // check 2D Idle BGAnim exists
		return true;
	return false;
}

void
Character::DemandGraphics()
{
//	++m_iPreloadRefcount;
//	if (m_iPreloadRefcount == 1) {
//		RString s = GetIconPath();
//		if (!s.empty())
//			m_Preload.Load(s);
//	}
}

void
Character::UndemandGraphics()
{
//	--m_iPreloadRefcount;
//	if (m_iPreloadRefcount == 0)
//		m_Preload.UnloadAll();
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Character. */
class LunaCharacter : public Luna<Character>
{
  public:
	static int GetCardPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetCardPath());
		return 1;
	}
	static int GetIconPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetIconPath());
		return 1;
	}
	static int GetSongSelectIconPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetSongSelectIconPath());
		return 1;
	}
	static int GetStageIconPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetStageIconPath());
		return 1;
	}
	static int GetModelPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetModelPath());
		return 1;
	}
	static int GetRestAnimationPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetRestAnimationPath());
		return 1;
	}
	static int GetWarmUpAnimationPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetWarmUpAnimationPath());
		return 1;
	}
	static int GetDanceAnimationPath(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDanceAnimationPath());
		return 1;
	}
	static int GetCharacterDir(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sCharDir);
		return 1;
	}
	static int GetCharacterID(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sCharacterID);
		return 1;
	}
	static int GetDisplayName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetDisplayName());
		return 1;
	}

	LunaCharacter()
	{
		ADD_METHOD(GetCardPath);
		ADD_METHOD(GetIconPath);
		ADD_METHOD(GetSongSelectIconPath);
		ADD_METHOD(GetStageIconPath);
		// sm-ssc adds:
		ADD_METHOD(GetModelPath);
		ADD_METHOD(GetRestAnimationPath);
		ADD_METHOD(GetWarmUpAnimationPath);
		ADD_METHOD(GetDanceAnimationPath);
		ADD_METHOD(GetCharacterDir);
		ADD_METHOD(GetCharacterID);
		ADD_METHOD(GetDisplayName);
	}
};

LUA_REGISTER_CLASS(Character)
// lua end
