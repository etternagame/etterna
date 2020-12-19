#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "ComboGraph.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Models/Misc/StageStats.h"
#include "Etterna/Singletons/PrefsManager.h"

#include <algorithm>

const int MinComboSizeToShow = 5;

REGISTER_ACTOR_CLASS(ComboGraph);

ComboGraph::ComboGraph()
{
	DeleteChildrenWhenDone(true);

	m_pNormalCombo = nullptr;
	m_pMaxCombo = nullptr;
	m_pComboNumber = nullptr;
	m_pBacking = nullptr;
}

void
ComboGraph::Load(const std::string& sMetricsGroup)
{
	BODY_WIDTH.Load(sMetricsGroup, "BodyWidth");
	BODY_HEIGHT.Load(sMetricsGroup, "BodyHeight");

	// These need to be set so that a theme can use zoomtowidth/zoomtoheight and
	// get correct behavior.
	this->SetWidth(BODY_WIDTH);
	this->SetHeight(BODY_HEIGHT);

	Actor* pActor = nullptr;

	m_pBacking =
	  ActorUtil::MakeActor(THEME->GetPathG(sMetricsGroup, "Backing"));
	if (m_pBacking != nullptr) {
		m_pBacking->ZoomToWidth(BODY_WIDTH);
		m_pBacking->ZoomToHeight(BODY_HEIGHT);
		this->AddChild(m_pBacking);
	}

	m_pNormalCombo =
	  ActorUtil::MakeActor(THEME->GetPathG(sMetricsGroup, "NormalCombo"));
	if (m_pNormalCombo != nullptr) {
		m_pNormalCombo->ZoomToWidth(BODY_WIDTH);
		m_pNormalCombo->ZoomToHeight(BODY_HEIGHT);
		this->AddChild(m_pNormalCombo);
	}

	m_pMaxCombo =
	  ActorUtil::MakeActor(THEME->GetPathG(sMetricsGroup, "MaxCombo"));
	if (m_pMaxCombo != nullptr) {
		m_pMaxCombo->ZoomToWidth(BODY_WIDTH);
		m_pMaxCombo->ZoomToHeight(BODY_HEIGHT);
		this->AddChild(m_pMaxCombo);
	}

	pActor =
	  ActorUtil::MakeActor(THEME->GetPathG(sMetricsGroup, "ComboNumber"));
	if (pActor != nullptr) {
		m_pComboNumber = dynamic_cast<BitmapText*>(pActor);
		if (m_pComboNumber != nullptr)
			this->AddChild(m_pComboNumber);
		else
			LuaHelpers::ReportScriptErrorFmt("ComboGraph: \"sMetricsGroup\" "
											 "\"ComboNumber\" must be a "
											 "BitmapText");
	}
}

void
ComboGraph::Set(const StageStats& s, const PlayerStageStats& pss)
{
	const float fLastSecond = s.GetTotalPossibleStepsSeconds();
	SetWithoutStageStats(pss, fLastSecond);
}

void
ComboGraph::SetWithoutStageStats(const PlayerStageStats& pss, const float fLastSecond)
{
	const float fFirstSecond = 0;

	// Unhide the templates.
	m_pNormalCombo->SetVisible(true);
	m_pMaxCombo->SetVisible(true);
	m_pComboNumber->SetVisible(true);

	// Find the largest combo.
	int iMaxComboSize = 0;
	for (unsigned i = 0; i < pss.m_ComboList.size(); ++i)
		iMaxComboSize =
		  std::max(iMaxComboSize, pss.m_ComboList[i].GetStageCnt());

	for (unsigned i = 0; i < pss.m_ComboList.size(); ++i) {
		const PlayerStageStats::Combo_t& combo = pss.m_ComboList[i];
		if (combo.GetStageCnt() < MinComboSizeToShow)
			continue; // too small

		const bool bIsMax = (combo.GetStageCnt() == iMaxComboSize);

		if (PREFSMAN->m_verbose_log > 1)
			Locator::getLogger()->trace("combo {} is {}+{} of {}",
										i,
										combo.m_fStartSecond,
										combo.m_fSizeSeconds,
										fLastSecond);
		Actor* pSprite = bIsMax ? m_pMaxCombo->Copy() : m_pNormalCombo->Copy();

		const float fStart =
		  SCALE(combo.m_fStartSecond, fFirstSecond, fLastSecond, 0.0f, 1.0f);
		const float fSize = SCALE(
		  combo.m_fSizeSeconds, 0, fLastSecond - fFirstSecond, 0.0f, 1.0f);
		pSprite->SetCropLeft(SCALE(fSize, 0.0f, 1.0f, 0.5f, 0.0f));
		pSprite->SetCropRight(SCALE(fSize, 0.0f, 1.0f, 0.5f, 0.0f));

		pSprite->SetCropLeft(fStart);
		pSprite->SetCropRight(1 - (fSize + fStart));

		this->AddChild(pSprite);
	}

	for (unsigned i = 0; i < pss.m_ComboList.size(); ++i) {
		const PlayerStageStats::Combo_t& combo = pss.m_ComboList[i];
		if (combo.GetStageCnt() < MinComboSizeToShow)
			continue; // too small

		if (!iMaxComboSize)
			continue;

		const bool bIsMax = (combo.GetStageCnt() == iMaxComboSize);
		if (!bIsMax)
			continue;

		BitmapText* pText = m_pComboNumber->Copy();

		const float fStart =
		  SCALE(combo.m_fStartSecond, fFirstSecond, fLastSecond, 0.0f, 1.0f);
		const float fSize = SCALE(
		  combo.m_fSizeSeconds, 0, fLastSecond - fFirstSecond, 0.0f, 1.0f);

		const float fCenterPercent = fStart + fSize / 2;
		const float fCenterXPos = SCALE(
		  fCenterPercent, 0.0f, 1.0f, -BODY_WIDTH / 2.0f, BODY_WIDTH / 2.0f);
		pText->SetX(fCenterXPos);

		pText->SetText(ssprintf("%i", combo.GetStageCnt()));

		this->AddChild(pText);
	}

	// Hide the templates.
	m_pNormalCombo->SetVisible(false);
	m_pMaxCombo->SetVisible(false);
	m_pComboNumber->SetVisible(false);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ComboGraph. */
class LunaComboGraph : public Luna<ComboGraph>
{
  public:
	static int Load(T* p, lua_State* L)
	{
		p->Load(SArg(1));
		COMMON_RETURN_SELF;
	}
	static int Set(T* p, lua_State* L)
	{
		StageStats* pStageStats = Luna<StageStats>::check(L, 1);
		PlayerStageStats* pPlayerStageStats =
		  Luna<PlayerStageStats>::check(L, 2);
		p->Set(*pStageStats, *pPlayerStageStats);
		COMMON_RETURN_SELF;
	}
	static int SetWithoutStageStats(T* p, lua_State* L)
	{
		const float lastsecond = FArg(2);
		auto* pPlayerStageStats =
		  Luna<PlayerStageStats>::check(L, 1);
		p->SetWithoutStageStats(*pPlayerStageStats, lastsecond);
		COMMON_RETURN_SELF;
	}
	static int Clear(T* p, lua_State* L)
	{
		p->DeleteAllChildren();
		COMMON_RETURN_SELF;
	}

	LunaComboGraph()
	{
		ADD_METHOD(Load);
		ADD_METHOD(Set);
		ADD_METHOD(SetWithoutStageStats);
		ADD_METHOD(Clear);
	}
};

LUA_REGISTER_DERIVED_CLASS(ComboGraph, ActorFrame)
// lua end
