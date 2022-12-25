#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "ScreenNetEvaluation.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Models/Misc/InputEventPlus.h"

#define USERSBG_WIDTH THEME->GetMetricF("ScreenNetEvaluation", "UsersBGWidth")
#define USERSBG_HEIGHT THEME->GetMetricF("ScreenNetEvaluation", "UsersBGHeight")
#define USERSBG_COMMAND                                                        \
	THEME->GetMetricA("ScreenNetEvaluation", "UsersBGCommand")

#define USERDX THEME->GetMetricF("ScreenNetEvaluation", "UserDX")
#define USERDY THEME->GetMetricF("ScreenNetEvaluation", "UserDY")

#define MAX_COMBO_NUM_DIGITS                                                   \
	THEME->GetMetricI("ScreenEvaluation", "MaxComboNumDigits")

REGISTER_SCREEN_CLASS(ScreenNetEvaluation);

void
ScreenNetEvaluation::Init()
{
	ScreenEvaluation::Init();
	m_iCurrentPlayer = 0;
	NSMAN->OnEval();
}

bool
ScreenNetEvaluation::Input(const InputEventPlus& input)
{
	// throw out "enter" inputs so players don't accidentally close the screen
	// while talking about scores, force them to esc to the next screen -mina
	if (input.DeviceI.button == KEY_ENTER)
		return false;

	return ScreenEvaluation::Input(input);
}

void
ScreenNetEvaluation::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_GoToNextScreen) {
		NSMAN->OffEval();
	}
	ScreenEvaluation::HandleScreenMessage(SM);
}

void
ScreenNetEvaluation::TweenOffScreen()
{
	ScreenEvaluation::TweenOffScreen();
}

void
ScreenNetEvaluation::UpdateStats()
{
	if (m_iCurrentPlayer >= static_cast<int>(NSMAN->m_EvalPlayerData.size()))
		return;

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
#include "Etterna/Models/Lua/LuaBinding.h"

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
			lua_pushstring(L,
						   NSMAN->m_EvalPlayerData[p->m_iCurrentPlayer]
							 .playerOptions.c_str());
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
	static int SetCurrentPlayerByName(T* p, lua_State* L)
	{
		std::string given = make_lower(SArg(1));
		for (size_t i = 0; i < NSMAN->m_EvalPlayerData.size(); i++) {
			EndOfGame_PlayerData& pd = NSMAN->m_EvalPlayerData[i];
			std::string name = make_lower(pd.nameStr);

			if (name == given) {
				p->m_iCurrentPlayer = static_cast<int>(i);
				p->UpdateStats();
				break;
			}
		}
		return 0;
	}
	LunaScreenNetEvaluation()
	{
		ADD_METHOD(GetNumActivePlayers);
		ADD_METHOD(GetHighScore);
		ADD_METHOD(GetOptions);
		ADD_METHOD(GetCurrentPlayer);
		ADD_METHOD(SetCurrentPlayer);
		ADD_METHOD(SetCurrentPlayerByName);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenNetEvaluation, ScreenEvaluation)
// lua end
