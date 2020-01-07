/* GameCommand */

#ifndef GameCommand_H
#define GameCommand_H

#include "Command.h"
#include "Difficulty.h"
#include "GameConstantsAndTypes.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "PlayerNumber.h"
#include <map>

class Song;
class Steps;
class Style;
struct Game;
struct lua_State;

class GameCommand
{
  public:
	GameCommand()
	  : m_Commands()
	  , m_sName("")
	  , m_sText("")
	  , m_sInvalidReason("")
	  , m_pStyle(NULL)
	  , m_sAnnouncer("")
	  , m_sPreferredModifiers("")
	  , m_sStageModifiers("")
	  , m_sScreen("")
	  , m_LuaFunction()
	  , m_pSong(NULL)
	  , m_pSteps(NULL)
	  , m_SetEnv()
	  , m_SetPref()
	  , m_sSongGroup("")
	  , m_sSoundPath("")
	  , m_vsScreensToPrepare()
	  , m_sProfileID("")
	  , m_sUrl("")
	{
		m_LuaFunction.Unset();
	}
	void Init();

	void Load(int iIndex, const Commands& cmds);
	void LoadOne(const Command& cmd);

	void ApplyToAllPlayers() const;
	void Apply(PlayerNumber pn) const;

  private:
	void Apply(const std::vector<PlayerNumber>& vpns) const;
	void ApplySelf(const std::vector<PlayerNumber>& vpns) const;

  public:
	bool DescribesCurrentMode(PlayerNumber pn) const;
	bool DescribesCurrentModeForAllPlayers() const;
	bool IsPlayable(RString* why = NULL) const;
	bool IsZero() const;

	/* If true, Apply() will apply m_sScreen. If false, it won't, and you need
	 * to do it yourself. */
	void ApplyCommitsScreens(bool bOn) { m_bApplyCommitsScreens = bOn; }

	// Same as what was passed to Load. We need to keep the original commands
	// so that we know the order of commands when it comes time to Apply.
	Commands m_Commands;

	RString m_sName; // choice name
	RString m_sText; // display text
	bool m_bInvalid{ true };
	RString m_sInvalidReason;
	int m_iIndex{ -1 };
	MultiPlayer m_MultiPlayer{ MultiPlayer_Invalid };
	const Style* m_pStyle;
	PlayMode m_pm{ PlayMode_Invalid };
	Difficulty m_dc{ Difficulty_Invalid };
	RString m_sAnnouncer;
	RString m_sPreferredModifiers;
	RString m_sStageModifiers;
	RString m_sScreen;
	LuaReference m_LuaFunction;
	Song* m_pSong;
	Steps* m_pSteps;
	std::map<RString, RString> m_SetEnv;
	std::map<RString, RString> m_SetPref;
	RString m_sSongGroup;
	SortOrder m_SortOrder{ SortOrder_Invalid };
	RString m_sSoundPath; // "" for no sound
	std::vector<RString> m_vsScreensToPrepare;
	RString m_sProfileID;
	RString m_sUrl;
	// sm-ssc adds:
	bool m_bUrlExits{ true }; // for making stepmania not exit on url

	bool m_bStopMusic{ false };
	bool m_bApplyDefaultOptions{ false };
	// sm-ssc also adds:
	bool m_bFadeMusic{ false };
	float m_fMusicFadeOutVolume{ -1 };
	// currently, GameSoundManager uses consts for fade out/in times, so this
	// is kind of pointless, but I want to have it working eventually. -aj
	float m_fMusicFadeOutSeconds{ -1 };

	// Lua
	void PushSelf(lua_State* L);

  private:
	bool m_bApplyCommitsScreens{ true };
};

#endif
