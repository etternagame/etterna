#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "ControllerStateDisplay.h"
#include "EnumHelper.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "RageUtil/Misc/RageInputDevice.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"

static const char* ControllerStateButtonNames[] = {
	"UpLeft", "UpRight", "Center", "DownLeft", "DownRight",
};
XToString(ControllerStateButton);

// TODO: Generalize for all game types
static const GameButton ControllerStateButtonToGameButton[] = {
	PUMP_BUTTON_UPLEFT,	  PUMP_BUTTON_UPRIGHT,	 PUMP_BUTTON_CENTER,
	PUMP_BUTTON_DOWNLEFT, PUMP_BUTTON_DOWNRIGHT,
};

REGISTER_ACTOR_CLASS(ControllerStateDisplay);

ControllerStateDisplay::ControllerStateDisplay()
{
	m_bIsLoaded = false;
	m_mp = MultiPlayer_Invalid;
	m_idsLast = InputDeviceState_Invalid;
}

void
ControllerStateDisplay::LoadMultiPlayer(const std::string& sType,
										MultiPlayer mp)
{
	LoadInternal(sType, mp, GameController_1);
}

void
ControllerStateDisplay::LoadGameController(const std::string& sType,
										   GameController gc)
{
	LoadInternal(sType, MultiPlayer_Invalid, gc);
}

void
ControllerStateDisplay::LoadInternal(const std::string& sType,
									 MultiPlayer mp,
									 GameController gc)
{
	ASSERT(!m_bIsLoaded);
	m_bIsLoaded = true;
	m_mp = mp;

	LuaThreadVariable varElement("MultiPlayer", LuaReference::Create(m_mp));
	m_sprFrame.Load(THEME->GetPathG(sType, "frame"));
	this->AddChild(m_sprFrame);

	FOREACH_ENUM(ControllerStateButton, b)
	{
		auto& button = m_Buttons[b];

		auto sPath = THEME->GetPathG(sType, ControllerStateButtonToString(b));
		button.spr.Load(sPath);
		this->AddChild(m_Buttons[b].spr);

		button.gi = GameInput(gc, ControllerStateButtonToGameButton[b]);
	}
}

void
ControllerStateDisplay::Update(float fDelta)
{
	ActorFrame::Update(fDelta);

	if (m_mp != MultiPlayer_Invalid) {
		auto id = InputMapper::MultiPlayerToInputDevice(m_mp);
		auto ids = INPUTMAN->GetInputDeviceState(id);
		if (ids != m_idsLast) {
			PlayCommand(InputDeviceStateToString(ids));
		}
		m_idsLast = ids;
	}

	FOREACH_ENUM(ControllerStateButton, b)
	{
		auto& button = m_Buttons[b];
		if (!button.spr.IsLoaded())
			continue;

		auto bVisible = INPUTMAPPER->IsBeingPressed(button.gi, m_mp);

		button.spr->SetVisible(bVisible);
	}
}

/** @brief Allow Lua to have access to the ControllerStateDisplay. */
class LunaControllerStateDisplay : public Luna<ControllerStateDisplay>
{
  public:
	static int LoadGameController(T* p, lua_State* L)
	{
		p->LoadGameController(SArg(1), Enum::Check<GameController>(L, 2));
		COMMON_RETURN_SELF;
	}
	static int LoadMultiPlayer(T* p, lua_State* L)
	{
		p->LoadMultiPlayer(SArg(1), Enum::Check<MultiPlayer>(L, 2));
		COMMON_RETURN_SELF;
	}

	LunaControllerStateDisplay()
	{
		ADD_METHOD(LoadGameController);
		ADD_METHOD(LoadMultiPlayer);
	}
};

LUA_REGISTER_DERIVED_CLASS(ControllerStateDisplay, ActorFrame)
