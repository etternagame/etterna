#include "global.h"

#if !defined(WITHOUT_NETWORKING)
#include "GameState.h"
#include "RageLog.h"
#include "ScreenNetEvaluation.h"
#include "SongUtil.h"
#include "Style.h"
#include "ThemeManager.h"
#include "NetworkSyncManager.h"
#include "InputEventPlus.h"

static const int NUM_SCORE_DIGITS = 9;

#define USERSBG_WIDTH THEME->GetMetricF("ScreenNetEvaluation", "UsersBGWidth")
#define USERSBG_HEIGHT THEME->GetMetricF("ScreenNetEvaluation", "UsersBGHeight")
#define USERSBG_COMMAND                                                        \
	THEME->GetMetricA("ScreenNetEvaluation", "UsersBGCommand")

#define USERDX THEME->GetMetricF("ScreenNetEvaluation", "UserDX")
#define USERDY THEME->GetMetricF("ScreenNetEvaluation", "UserDY")

#define MAX_COMBO_NUM_DIGITS                                                   \
	THEME->GetMetricI("ScreenEvaluation", "MaxComboNumDigits")

static AutoScreenMessage(SM_GotEval);
AutoScreenMessage(ETTP_NewScore);

REGISTER_SCREEN_CLASS(ScreenNetEvaluation);

void
ScreenNetEvaluation::Init()
{
	ScreenEvaluation::Init();

	m_bHasStats = false;
	m_iCurrentPlayer = 0;

	NSMAN->OnEval();
}

bool
ScreenNetEvaluation::MenuLeft(const InputEventPlus& input)
{
	return MenuUp(input);
}

bool
ScreenNetEvaluation::MenuUp(const InputEventPlus& input)
{
	if (m_iActivePlayers == 0 || !m_bHasStats)
		return false;

	COMMAND(m_textUsers[m_iCurrentPlayer], "DeSel");
	m_iCurrentPlayer =
	  (m_iCurrentPlayer + m_iActivePlayers - 1) % m_iActivePlayers;
	COMMAND(m_textUsers[m_iCurrentPlayer], "Sel");
	UpdateStats();
	return true;
}

bool
ScreenNetEvaluation::MenuRight(const InputEventPlus& input)
{
	return MenuDown(input);
}

bool
ScreenNetEvaluation::MenuDown(const InputEventPlus& input)
{
	if (m_iActivePlayers == 0 || !m_bHasStats)
		return false;

	COMMAND(m_textUsers[m_iCurrentPlayer], "DeSel");
	m_iCurrentPlayer = (m_iCurrentPlayer + 1) % m_iActivePlayers;
	COMMAND(m_textUsers[m_iCurrentPlayer], "Sel");
	UpdateStats();
	return true;
}

bool
ScreenNetEvaluation::Input(const InputEventPlus& input)
{
	// throw out "enter" inputs so players don't accidentally close the screen
	// while talking about scores, force them to esc to the next screen -mina
	if (input.DeviceI.button == KEY_ENTER)
		return false;

	return Screen::Input(input);
}

void
ScreenNetEvaluation::HandleScreenMessage(const ScreenMessage SM)
{
	if (SM == SM_GotEval || SM == ETTP_NewScore) {
		m_bHasStats = true;

		LOG->Trace("[SMNETDebug] num active players: %d (local), %d (NSMAN)",
				   m_iActivePlayers,
				   NSMAN->m_ActivePlayers);

		LOG->Trace("SMNETCheckpoint");
		for (int i = 0; i < m_iActivePlayers; ++i) {
			// Strange occurences because of timing cause these things not to
			// work right and will sometimes cause a crash. We should make SURE
			// we won't crash!
			if (size_t(i) >= NSMAN->m_EvalPlayerData.size())
				break;

			if (NSMAN->m_EvalPlayerData[i].nameStr.empty() &&
				size_t(NSMAN->m_EvalPlayerData[i].name) >=
				  NSMAN->m_PlayerNames.size())
				break;

			if (NSMAN->m_EvalPlayerData[i].nameStr.empty() &&
				NSMAN->m_EvalPlayerData[i].name < 0)
				break;

			if (size_t(i) >= m_textUsers.size())
				break;

			m_textUsers[i].SetText(
			  NSMAN->m_EvalPlayerData[i].nameStr.empty()
				? NSMAN->m_PlayerNames[NSMAN->m_EvalPlayerData[i].name]
				: NSMAN->m_EvalPlayerData[i].nameStr);

			if (NSMAN->m_EvalPlayerData[m_iCurrentPlayer].hs.GetWifeGrade() <
				Grade_Tier03)
				m_textUsers[i].PlayCommand("Tier02OrBetter");

			ON_COMMAND(m_textUsers[i]);
			LOG->Trace("SMNETCheckpoint%d", i);
		}
		return; // No need to let ScreenEvaluation get a hold of this.
	}
	if (SM == SM_GoToNextScreen) {
		NSMAN->OffEval();
	}
	ScreenEvaluation::HandleScreenMessage(SM);
}

void
ScreenNetEvaluation::TweenOffScreen()
{
	for (int i = 0; i < m_iActivePlayers; ++i)
		OFF_COMMAND(m_textUsers[i]);
	OFF_COMMAND(m_rectUsersBG);
	ScreenEvaluation::TweenOffScreen();
}

void
ScreenNetEvaluation::UpdateStats()
{
	if (m_iCurrentPlayer >= (int)NSMAN->m_EvalPlayerData.size())
		return;

	// Only run these commands if the theme has these things shown; not every
	// theme has them, so don't assume. -aj
	if (THEME->GetMetricB(m_sName, "ShowGradeArea"))
		m_Grades[m_pActivePlayer].SetGrade(static_cast<Grade>(
		  NSMAN->m_EvalPlayerData[m_iCurrentPlayer].hs.GetGrade() !=
			  Grade_NoData
			? NSMAN->m_EvalPlayerData[m_iCurrentPlayer].hs.GetGrade()
			: NSMAN->m_EvalPlayerData[m_iCurrentPlayer].grade));
	if (THEME->GetMetricB(m_sName, "ShowScoreArea"))
		m_textScore[m_pActivePlayer].SetTargetNumber(
		  static_cast<float>(NSMAN->m_EvalPlayerData[m_iCurrentPlayer].score));

	// Values greater than 6 will cause a crash
	if (NSMAN->m_EvalPlayerData[m_iCurrentPlayer].difficulty < 6) {
		m_DifficultyIcon[m_pActivePlayer].SetPlayer(m_pActivePlayer);
		m_DifficultyIcon[m_pActivePlayer].SetFromDifficulty(
		  NSMAN->m_EvalPlayerData[m_iCurrentPlayer].difficulty);
	}

	for (int j = 0; j < NETNUMTAPSCORES; ++j) {
		// The name will be blank if ScreenEvaluation determined the line
		// should not be shown.
		if (!m_textJudgmentLineNumber[j][m_pActivePlayer].GetName().empty()) {
			m_textJudgmentLineNumber[j][m_pActivePlayer].SetTargetNumber(
			  static_cast<float>(
				NSMAN->m_EvalPlayerData[m_iCurrentPlayer].tapScores[j]));
		}
	}

	m_textPlayerOptions[m_pActivePlayer].SetText(
	  NSMAN->m_EvalPlayerData[m_iCurrentPlayer].playerOptions);

	StepsType st = GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType;
	Difficulty dc = NSMAN->m_EvalPlayerData[m_iCurrentPlayer].difficulty;
	Steps* pSteps = SongUtil::GetOneSteps(GAMESTATE->m_pCurSong, st, dc);

	// broadcast a message so themes know that the active player has changed.
	// -aj
	Message msg("UpdateNetEvalStats");
	msg.SetParam("ActivePlayerIndex", m_pActivePlayer);
	msg.SetParam("Difficulty",
				 NSMAN->m_EvalPlayerData[m_iCurrentPlayer].difficulty);
	msg.SetParam("Score", NSMAN->m_EvalPlayerData[m_iCurrentPlayer].score);
	msg.SetParam("Grade",
				 NSMAN->m_EvalPlayerData[m_iCurrentPlayer].hs.GetWifeGrade());
	msg.SetParam("PlayerOptions",
				 NSMAN->m_EvalPlayerData[m_iCurrentPlayer].playerOptions);
	if (pSteps)
		msg.SetParam("Steps", pSteps);
	MESSAGEMAN->Broadcast(msg);
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenNetEvaluation. */
class LunaScreenNetEvaluation : public Luna<ScreenNetEvaluation>
{
  public:
	static int GetNumActivePlayers(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumActivePlayers());
		return 1;
	}
	static int GetHighScore(T* p, lua_State* L)
	{
		if (static_cast<int>(NSMAN->m_EvalPlayerData.size()) - 1 >=
			p->m_iCurrentPlayer)
			NSMAN->m_EvalPlayerData[p->m_iCurrentPlayer].hs.PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetOptions(T* p, lua_State* L)
	{
		if (static_cast<int>(NSMAN->m_EvalPlayerData.size()) - 1 >=
			p->m_iCurrentPlayer)
			lua_pushstring(
			  L, NSMAN->m_EvalPlayerData[p->m_iCurrentPlayer].playerOptions);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetCurrentPlayer(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iCurrentPlayer + 1);
		return 1;
	}
	static int SetCurrentPlayer(T* p, lua_State* L)
	{
		p->m_iCurrentPlayer = IArg(1) - 1;
		p->UpdateStats();
		return 0;
	}
	LunaScreenNetEvaluation()
	{
		ADD_METHOD(GetNumActivePlayers);
		ADD_METHOD(GetHighScore);
		ADD_METHOD(GetOptions);
		ADD_METHOD(GetCurrentPlayer);
		ADD_METHOD(SetCurrentPlayer);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenNetEvaluation, ScreenEvaluation)
// lua end

#endif

/*
 * (c) 2004-2005 Charles Lohr, Joshua Allen
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
