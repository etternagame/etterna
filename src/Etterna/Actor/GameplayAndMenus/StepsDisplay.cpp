#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "StepsDisplay.h"

#include <algorithm>

REGISTER_ACTOR_CLASS(StepsDisplay);

StepsDisplay::StepsDisplay() = default;

/* sID experiment:
 *
 * Names of an actor, "Foo":
 * [Foo]
 * Metric=abc
 *
 * [ScreenSomething]
 * FooP1X=20
 * FooP2Y=30
 *
 * Graphics\Foo under p1
 *
 * We want to call it different things in different contexts: we may only want
 * one set of internal metrics for a given use, but separate metrics for each
 * player at the screen level, and we may or may not want separate names at the
 * asset level.
 *
 * As is, we tend to end up having to either duplicate [Foo] to [FooP1] and
 * [FooP2] or not use m_sName for [Foo], which limits its use.  Let's try using
 * a separate name for internal metrics.  I'm not sure if this will cause more
 * confusion than good, so I'm trying it first in only this object.
 */

void
StepsDisplay::Load(const std::string& sMetricsGroup,
				   const PlayerState* pPlayerState)
{
	m_sMetricsGroup = sMetricsGroup;

	/* We can't use global ThemeMetric<std::string>s, because we can have
	 * multiple StepsDisplays on screen at once, with different names. */
	m_iNumTicks.Load(m_sMetricsGroup, "NumTicks");
	m_iMaxTicks.Load(m_sMetricsGroup, "MaxTicks");
	m_bShowTicks.Load(m_sMetricsGroup, "ShowTicks");
	m_bShowMeter.Load(m_sMetricsGroup, "ShowMeter");
	m_bShowDescription.Load(m_sMetricsGroup, "ShowDescription");
	m_bShowCredit.Load(m_sMetricsGroup, "ShowCredit");
	m_bShowStepsType.Load(m_sMetricsGroup, "ShowStepsType");
	m_sZeroMeterString.Load(m_sMetricsGroup, "ZeroMeterString");
	m_sMeterFormatString.Load(m_sMetricsGroup, "MeterFormatString");

	m_sprFrame.Load(THEME->GetPathG(m_sMetricsGroup, "frame"));
	m_sprFrame->SetName("Frame");
	ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(m_sprFrame, m_sMetricsGroup);
	this->AddChild(m_sprFrame);

	if (m_bShowTicks) {
		std::string sChars = "10"; // on, off (todo: make this metricable -aj)
		m_textTicks.SetName("Ticks");
		m_textTicks.LoadFromTextureAndChars(
		  THEME->GetPathF(m_sMetricsGroup, "ticks"), sChars);
		ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(m_textTicks,
													   m_sMetricsGroup);
		this->AddChild(&m_textTicks);
	}

	if (m_bShowMeter) {
		m_textMeter.SetName("Meter");
		m_textMeter.LoadFromFont(THEME->GetPathF(m_sMetricsGroup, "meter"));
		ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(m_textMeter,
													   m_sMetricsGroup);
		this->AddChild(&m_textMeter);

		// These commands should have been loaded by SetXYAndOnCommand above.
		ASSERT(m_textMeter.HasCommand("Set"));
	}

	if (m_bShowDescription) {
		m_textDescription.SetName("Description");
		m_textDescription.LoadFromFont(
		  THEME->GetPathF(m_sMetricsGroup, "Description"));
		ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(m_textDescription,
													   m_sMetricsGroup);
		this->AddChild(&m_textDescription);
	}
	if (m_bShowCredit) {
		m_textAuthor.SetName("Step Author");
		m_textAuthor.LoadFromFont(THEME->GetPathF(m_sMetricsGroup, "Credit"));
		ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(m_textAuthor,
													   m_sMetricsGroup);
		this->AddChild(&m_textAuthor);
	}

	if (m_bShowStepsType) {
		m_sprStepsType.Load(THEME->GetPathG(m_sMetricsGroup, "StepsType"));
		m_sprStepsType->SetName("StepsType");
		ActorUtil::LoadAllCommandsAndSetXYAndOnCommand(m_sprStepsType,
													   m_sMetricsGroup);
		this->AddChild(m_sprStepsType);
	}

	// Play Load Command
	auto* pPlayerState_ = const_cast<PlayerState*>(pPlayerState);
	Message msg("Load");
	if (pPlayerState_)
		msg.SetParam("PlayerState",
					 LuaReference::CreateFromPush(*pPlayerState_));
	this->HandleMessage(msg);

	Unset();
}

void
StepsDisplay::SetFromGameState(PlayerNumber pn)
{
	const Steps* pSteps = GAMESTATE->m_pCurSteps;
	if (pSteps != nullptr)
		SetFromSteps(pSteps);
	else
		SetFromStepsTypeAndMeterAndDifficultyAndCourseType(
		  StepsType_Invalid, 0, GAMESTATE->m_PreferredDifficulty);
}

void
StepsDisplay::SetFromSteps(const Steps* pSteps)
{
	if (pSteps == nullptr) {
		Unset();
		return;
	}

	SetParams params = {
		pSteps, pSteps->GetMeter(), pSteps->m_StepsType, pSteps->GetDifficulty()
	};
	SetInternal(params);
}

void
StepsDisplay::Unset()
{
	this->SetVisible(false);
}

void
StepsDisplay::SetFromStepsTypeAndMeterAndDifficultyAndCourseType(StepsType st,
																 int iMeter,
																 Difficulty dc)
{
	SetParams params = { nullptr, iMeter, st, dc };
	SetInternal(params);
}

void
StepsDisplay::SetInternal(const SetParams& params)
{
	this->SetVisible(true);
	Message msg("Set");

	std::string sCustomDifficulty;
	if (params.pSteps)
		sCustomDifficulty = StepsToCustomDifficulty(params.pSteps);
	else
		sCustomDifficulty = GetCustomDifficulty(params.st, params.dc);
	msg.SetParam("CustomDifficulty", sCustomDifficulty);

	std::string sDisplayDescription;

	if (sCustomDifficulty.empty())
		sDisplayDescription = std::string();
	else
		sDisplayDescription =
		  CustomDifficultyToLocalizedString(sCustomDifficulty);
	msg.SetParam("DisplayDescription", sDisplayDescription);

	std::string sDisplayCredit;
	if (params.pSteps)
		sDisplayCredit = params.pSteps->GetCredit();

	if (params.pSteps)
		msg.SetParam("Steps",
					 LuaReference::CreateFromPush(*(Steps*)params.pSteps));
	msg.SetParam("Meter", params.iMeter);
	msg.SetParam("StepsType", params.st);

	m_sprFrame->HandleMessage(msg);

	if (m_bShowTicks) {
		// todo: let themers handle the logic of tick text. -aj
		auto on = static_cast<char>('1');
		auto off = '0';

		std::string sNewText;
		auto iNumOn = std::min(static_cast<int>(m_iMaxTicks), params.iMeter);
		sNewText.insert(sNewText.end(), iNumOn, on);
		auto iNumOff = std::max(0, m_iNumTicks - iNumOn);
		sNewText.insert(sNewText.end(), iNumOff, off);
		m_textTicks.SetText(sNewText);
		m_textTicks.HandleMessage(msg);
	}

	if (m_bShowMeter) {
		if (params.iMeter == 0) // Unset calls with this
		{
			m_textMeter.SetText(m_sZeroMeterString);
		} else {
			const std::string sMeter =
			  ssprintf(m_sMeterFormatString.GetValue().c_str(), params.iMeter);
			m_textMeter.SetText(sMeter);
			m_textMeter.HandleMessage(msg);
		}
	}

	if (m_bShowDescription) {
		m_textDescription.SetText(sDisplayDescription);
		m_textDescription.HandleMessage(msg);
	}
	if (m_bShowCredit) {
		m_textAuthor.SetText(sDisplayCredit);
		m_textAuthor.HandleMessage(msg);
	}
	if (m_bShowStepsType) {
		if (params.st != StepsType_Invalid) {
			/*
			std::string sStepsType =
			GAMEMAN->GetStepsTypeInfo(params.st).szName; m_sprStepsType.Load(
			THEME->GetPathG(m_sMetricsGroup,"StepsType
			"+sStepsType) );
			*/
			m_sprStepsType->HandleMessage(msg);
		}
	}

	this->HandleMessage(msg);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the StepsDisplay. */
class LunaStepsDisplay : public Luna<StepsDisplay>
{
  public:
	static int Load(T* p, lua_State* L)
	{
		p->Load(SArg(1), nullptr);
		COMMON_RETURN_SELF;
	}
	static int SetFromSteps(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			p->SetFromSteps(nullptr);
		} else {
			auto pS = Luna<Steps>::check(L, 1);
			p->SetFromSteps(pS);
		}
		COMMON_RETURN_SELF;
	}
	static int SetFromGameState(T* p, lua_State* L)
	{
		auto pn = PLAYER_1;
		p->SetFromGameState(pn);
		COMMON_RETURN_SELF;
	}
	static int GetIndex(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->mypos);
		return 1;
	}

	LunaStepsDisplay()
	{
		ADD_METHOD(Load);
		ADD_METHOD(SetFromSteps);
		ADD_METHOD(SetFromGameState);
		ADD_METHOD(GetIndex);
	}
};

LUA_REGISTER_DERIVED_CLASS(StepsDisplay, ActorFrame)
// lua end
