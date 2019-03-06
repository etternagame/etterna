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
class Character;
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
	  , m_pCharacter(NULL)
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
	void Apply(const vector<PlayerNumber>& vpns) const;
	void ApplySelf(const vector<PlayerNumber>& vpns) const;

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
	Character* m_pCharacter;
	std::map<RString, RString> m_SetEnv;
	std::map<RString, RString> m_SetPref;
	RString m_sSongGroup;
	SortOrder m_SortOrder{ SortOrder_Invalid };
	RString m_sSoundPath; // "" for no sound
	vector<RString> m_vsScreensToPrepare;
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

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
