#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "HoldJudgment.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/FileTypes/XmlFile.h"

REGISTER_ACTOR_CLASS(HoldJudgment);

HoldJudgment::HoldJudgment()
{
	m_mpToTrack = MultiPlayer_Invalid;
}

void
HoldJudgment::Load(const std::string& sPath)
{
	m_sprJudgment.Load(sPath);
	m_sprJudgment->StopAnimating();
	m_sprJudgment->SetName("HoldJudgment");
	ActorUtil::LoadAllCommands(*m_sprJudgment, "HoldJudgment");
	ResetAnimation();
	this->AddChild(m_sprJudgment);
}

void
HoldJudgment::LoadFromNode(const XNode* pNode)
{
	std::string sFile;
	if (!ActorUtil::GetAttrPath(pNode, "File", sFile)) {
		LuaHelpers::ReportScriptErrorFmt(
		  "%s: HoldJudgment: missing the attribute \"File\"",
		  ActorUtil::GetWhere(pNode).c_str());
	}

	CollapsePath(sFile);

	Load(sFile);

	ActorFrame::LoadFromNode(pNode);
}

void
HoldJudgment::ResetAnimation()
{
	ASSERT(m_sprJudgment.IsLoaded());
	m_sprJudgment->SetDiffuse(RageColor(1, 1, 1, 0));
	m_sprJudgment->SetXY(0, 0);
	m_sprJudgment->StopTweening();
	m_sprJudgment->StopEffect();
}

void
HoldJudgment::SetHoldJudgment(HoldNoteScore hns)
{
	// LOG->Trace( "Judgment::SetJudgment()" );

	// Matt: To save API. Command can handle if desired.
	if (hns != HNS_Missed) {
		ResetAnimation();
	}

	switch (hns) {
		case HNS_Held:
			m_sprJudgment->SetState(0);
			m_sprJudgment->PlayCommand("Held");
			break;
		case HNS_LetGo:
			m_sprJudgment->SetState(1);
			m_sprJudgment->PlayCommand("LetGo");
			break;
		case HNS_Missed:
			// m_sprJudgment->SetState( 2 ); // Matt: Not until after 5.0
			m_sprJudgment->PlayCommand("MissedHold");
			break;
		case HNS_None:
		default:
			FAIL_M(ssprintf("Cannot set hold judgment to %i", hns));
	}
}

void
HoldJudgment::LoadFromMultiPlayer(MultiPlayer mp)
{
	ASSERT(m_mpToTrack == MultiPlayer_Invalid); // assert only load once
	m_mpToTrack = mp;
	this->SubscribeToMessage("Judgment");
}

void
HoldJudgment::HandleMessage(const Message& msg)
{
	if (m_mpToTrack != MultiPlayer_Invalid && msg.GetName() == "Judgment") {
		MultiPlayer mp;
		if (msg.GetParam("MultiPlayer", mp) && mp == m_mpToTrack) {
			HoldNoteScore hns;
			if (msg.GetParam("HoldNoteScore", hns))
				SetHoldJudgment(hns);
		}
	}

	ActorFrame::HandleMessage(msg);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the HoldJudgment. */
class LunaHoldJudgment : public Luna<HoldJudgment>
{
  public:
	static int LoadFromMultiPlayer(T* p, lua_State* L)
	{
		p->LoadFromMultiPlayer(Enum::Check<MultiPlayer>(L, 1));
		COMMON_RETURN_SELF;
	}

	LunaHoldJudgment() { ADD_METHOD(LoadFromMultiPlayer); }
};

LUA_REGISTER_DERIVED_CLASS(HoldJudgment, ActorFrame)
// lua end
