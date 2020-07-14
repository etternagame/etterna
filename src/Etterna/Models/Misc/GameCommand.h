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
	  : m_sName("")
	  , m_sText("")
	  , m_sInvalidReason("")
	  , m_sAnnouncer("")
	  , m_sPreferredModifiers("")
	  , m_sStageModifiers("")
	  , m_sScreen("")
	  , m_sSongGroup("")
	  , m_sSoundPath("")
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
	void Apply(const vector<PlayerNumber>& vpns) const;
	void ApplySelf(const vector<PlayerNumber>& vpns) const;

  public:
	[[nodiscard]] auto DescribesCurrentMode(PlayerNumber pn) const -> bool;
	[[nodiscard]] auto DescribesCurrentModeForAllPlayers() const -> bool;
	auto IsPlayable(std::string* why = nullptr) const -> bool;
	[[nodiscard]] auto IsZero() const -> bool;

	/* If true, Apply() will apply m_sScreen. If false, it won't, and you need
	 * to do it yourself. */
	void ApplyCommitsScreens(bool bOn) { m_bApplyCommitsScreens = bOn; }

	// Same as what was passed to Load. We need to keep the original commands
	// so that we know the order of commands when it comes time to Apply.
	Commands m_Commands;

	std::string m_sName; // choice name
	std::string m_sText; // display text
	bool m_bInvalid{ true };
	std::string m_sInvalidReason;
	int m_iIndex{ -1 };
	MultiPlayer m_MultiPlayer{ MultiPlayer_Invalid };
	const Style* m_pStyle{ nullptr };
	Difficulty m_dc{ Difficulty_Invalid };
	std::string m_sAnnouncer;
	std::string m_sPreferredModifiers;
	std::string m_sStageModifiers;
	std::string m_sScreen;
	LuaReference m_LuaFunction;
	Song* m_pSong{ nullptr };
	Steps* m_pSteps{ nullptr };
	std::map<std::string, std::string> m_SetEnv;
	std::map<std::string, std::string> m_SetPref;
	std::string m_sSongGroup;
	SortOrder m_SortOrder{ SortOrder_Invalid };
	std::string m_sSoundPath; // "" for no sound
	vector<std::string> m_vsScreensToPrepare;
	std::string m_sProfileID;
	std::string m_sUrl;
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
