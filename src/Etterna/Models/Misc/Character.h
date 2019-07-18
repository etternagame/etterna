#ifndef Character_H
#define Character_H

#include "Etterna/Models/Lua/LuaReference.h"
struct lua_State;
using Lua = lua_State;

/** @brief A persona that defines attacks for use in battle. */
class Character
{
  public:
	Character();
	~Character() = default;

	bool Load(RString sCharDir); // return true if successful

	RString GetTakingABreakPath() const;
	RString GetCardPath() const { return m_sCardPath; }
	RString GetIconPath() const { return m_sIconPath; }

	RString GetModelPath() const;
	RString GetRestAnimationPath() const;
	RString GetWarmUpAnimationPath() const;
	RString GetDanceAnimationPath() const;
	RString GetSongSelectIconPath() const;
	RString GetStageIconPath() const;
	bool Has2DElems();

	bool IsDefaultCharacter() const
	{
		return m_sCharacterID.CompareNoCase("default") == 0;
	}

	void DemandGraphics();
	void UndemandGraphics();

	// Lua
	void PushSelf(Lua* L);

	// smart accessor
	const RString& GetDisplayName()
	{
		return !m_sDisplayName.empty() ? m_sDisplayName : m_sCharacterID;
	}

	RString m_sCharDir;
	RString m_sCharacterID;

  private:
	RString m_sDisplayName;
	RString m_sCardPath;
	RString m_sIconPath;

  public:
	apActorCommands m_cmdInit;
	int m_iPreloadRefcount{ 0 };
};

#endif
