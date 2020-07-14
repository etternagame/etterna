#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/GameState.h"
#include "LifeMeterBar.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Actor/GameplayAndMenus/StreamDisplay.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

#include <algorithm>

static std::string
LIFE_PERCENT_CHANGE_NAME(size_t i)
{
	return "LifePercentChange" + ScoreEventToString(static_cast<ScoreEvent>(i));
}

float
LifeMeterBar::MapTNSToDeltaLife(TapNoteScore score)
{
	auto fDeltaLife = 0.f;
	switch (score) {
		DEFAULT_FAIL(score);

			// Using constant values here as a hack
			// For estimating replay stuff

		case TNS_W1:
			fDeltaLife = 0.008f;
			break;
		case TNS_W2:
			fDeltaLife = 0.008f;
			break;
		case TNS_W3:
			fDeltaLife = 0.004f;
			break;
		case TNS_W4:
			fDeltaLife = 0.f;
			break;
		case TNS_W5:
			fDeltaLife = -0.04f;
			break;
		case TNS_Miss:
			fDeltaLife = -0.08f;
			break;
		case TNS_HitMine:
			fDeltaLife = -0.16f;
			break;
		case TNS_None:
			fDeltaLife = -0.08f;
			break;
		case TNS_CheckpointHit:
			fDeltaLife = 0.008f;
			break;
		case TNS_CheckpointMiss:
			fDeltaLife = -0.08f;
			break;
	}
	if (fDeltaLife > 0)
		fDeltaLife *= PREFSMAN->m_fLifeDifficultyScale;
	else
		fDeltaLife /= PREFSMAN->m_fLifeDifficultyScale;
	return fDeltaLife;
}

float
LifeMeterBar::MapHNSToDeltaLife(HoldNoteScore score)
{
	auto fDeltaLife = 0.f;
	switch (score) {
		case HNS_Held:
			fDeltaLife = 0.f;
			break;
		case HNS_LetGo:
			fDeltaLife = -0.08f;
			break;
		case HNS_Missed:
			fDeltaLife = 0.f;
			break;
		default:
			FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
	}
	if (fDeltaLife > 0)
		fDeltaLife *= PREFSMAN->m_fLifeDifficultyScale;
	else
		fDeltaLife /= PREFSMAN->m_fLifeDifficultyScale;
	return fDeltaLife;
}

LifeMeterBar::LifeMeterBar()
{
	DANGER_THRESHOLD.Load("LifeMeterBar", "DangerThreshold");
	INITIAL_VALUE.Load("LifeMeterBar", "InitialValue");
	HOT_VALUE.Load("LifeMeterBar", "HotValue");
	LIFE_MULTIPLIER.Load("LifeMeterBar", "LifeMultiplier");
	MIN_STAY_ALIVE.Load("LifeMeterBar", "MinStayAlive");
	m_fLifePercentChange.Load(
	  "LifeMeterBar", LIFE_PERCENT_CHANGE_NAME, NUM_ScoreEvent);
	m_pPlayerState = nullptr;

	const std::string sType = "LifeMeterBar";

	m_fPassingAlpha = 0;
	m_fHotAlpha = 0;

	m_fBaseLifeDifficulty = PREFSMAN->m_fLifeDifficultyScale;
	m_fLifeDifficulty = m_fBaseLifeDifficulty;

	// set up progressive lifebar
	m_iMissCombo = 0;

	// set up combotoregainlife
	m_iComboToRegainLife = 0;

	m_sprUnder.Load(THEME->GetPathG(sType, "Under"));
	m_sprUnder->SetName("Under");
	ActorUtil::LoadAllCommandsAndSetXY(m_sprUnder, sType);
	this->AddChild(m_sprUnder);

	m_sprDanger.Load(THEME->GetPathG(sType, "Danger"));
	m_sprDanger->SetName("Danger");
	ActorUtil::LoadAllCommandsAndSetXY(m_sprDanger, sType);
	this->AddChild(m_sprDanger);

	m_pStream = new StreamDisplay;
	m_pStream->Load("StreamDisplay");
	m_pStream->SetName("Stream");
	ActorUtil::LoadAllCommandsAndSetXY(m_pStream, sType);
	this->AddChild(m_pStream);

	m_sprOver.Load(THEME->GetPathG(sType, "Over"));
	m_sprOver->SetName("Over");
	ActorUtil::LoadAllCommandsAndSetXY(m_sprOver, sType);
	this->AddChild(m_sprOver);

	m_Change_SE_W1 = m_fLifePercentChange.GetValue(SE_W1);
	m_Change_SE_W2 = m_fLifePercentChange.GetValue(SE_W2);
	m_Change_SE_W3 = m_fLifePercentChange.GetValue(SE_W3);
	m_Change_SE_W4 = m_fLifePercentChange.GetValue(SE_W4);
	m_Change_SE_W5 = m_fLifePercentChange.GetValue(SE_W5);
	m_Change_SE_Miss = m_fLifePercentChange.GetValue(SE_Miss);
	m_Change_SE_HitMine = m_fLifePercentChange.GetValue(SE_HitMine);
	m_Change_SE_CheckpointHit = m_fLifePercentChange.GetValue(SE_CheckpointHit);
	m_Change_SE_CheckpointMiss =
	  m_fLifePercentChange.GetValue(SE_CheckpointMiss);
	m_Change_SE_Held = m_fLifePercentChange.GetValue(SE_Held);
	m_Change_SE_LetGo = m_fLifePercentChange.GetValue(SE_LetGo);
	m_fLifePercentage = INITIAL_VALUE;
	m_iProgressiveLifebar = 0;
}

LifeMeterBar::~LifeMeterBar()
{
	SAFE_DELETE(m_pStream);
}

void
LifeMeterBar::Load(const PlayerState* pPlayerState,
				   PlayerStageStats* pPlayerStageStats)
{
	LifeMeter::Load(pPlayerState, pPlayerStageStats);

	const auto dtype = pPlayerState->m_PlayerOptions.GetStage().m_DrainType;
	switch (dtype) {
		case DrainType_Normal:
			m_fLifePercentage = INITIAL_VALUE;
			break;
			/* These types only go down, so they always start at full. */
		case DrainType_NoRecover:
		case DrainType_SuddenDeath:
			m_fLifePercentage = 1.0f;
			break;
		default:
			FAIL_M(ssprintf("Invalid DrainType: %i", dtype));
	}

	AfterLifeChanged();
}

void
LifeMeterBar::ChangeLife(TapNoteScore score)
{
	auto fDeltaLife = 0.f;
	switch (score) {
		DEFAULT_FAIL(score);

			/*Whatever was originally done to try and be fancy resulted in the
			lifemeterbar parsing the metrics file to obtain constant values
			every single update cycle. This is obviously a temporary fix however
			the 200 avg fps gain is well worth it. Also, this function should
			only be called if life values actually change, like with the
			lifechanged message. - Mina*/

		case TNS_W1:
			fDeltaLife = m_Change_SE_W1;
			break;
		case TNS_W2:
			fDeltaLife = m_Change_SE_W2;
			break;
		case TNS_W3:
			fDeltaLife = m_Change_SE_W3;
			break;
		case TNS_W4:
			fDeltaLife = m_Change_SE_W4;
			break;
		case TNS_W5:
			fDeltaLife = m_Change_SE_W5;
			break;
		case TNS_Miss:
			fDeltaLife = m_Change_SE_Miss;
			break;
		case TNS_HitMine:
			fDeltaLife = m_Change_SE_HitMine;
			break;
		case TNS_None:
			fDeltaLife = m_Change_SE_Miss;
			break;
		case TNS_CheckpointHit:
			fDeltaLife = m_Change_SE_CheckpointHit;
			break;
		case TNS_CheckpointMiss:
			fDeltaLife = m_Change_SE_CheckpointMiss;
			break;
	}

	switch (m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType) {
		DEFAULT_FAIL(m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType);
		case DrainType_Normal:
			break;
		case DrainType_NoRecover:
			fDeltaLife = std::min(fDeltaLife, 0.F);
			break;
		case DrainType_SuddenDeath:
			if (score < MIN_STAY_ALIVE)
				fDeltaLife = -1.0f;
			else
				fDeltaLife = 0;
			break;
	}

	ChangeLife(fDeltaLife);
}

void
LifeMeterBar::ChangeLife(HoldNoteScore score, TapNoteScore tscore)
{
	auto fDeltaLife = 0.f;
	const auto dtype = m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType;
	switch (dtype) {
		case DrainType_Normal:
			switch (score) {
				case HNS_Held:
					fDeltaLife = m_Change_SE_Held;
					break;
				case HNS_LetGo:
					fDeltaLife = m_Change_SE_LetGo;
					break;
				case HNS_Missed:
					fDeltaLife = 0.f;
					break;
				default:
					FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
			}
			break;
		case DrainType_NoRecover:
			switch (score) {
				case HNS_Held:
					fDeltaLife = +0.000f;
					break;
				case HNS_LetGo:
					fDeltaLife = m_Change_SE_LetGo;
					break;
				case HNS_Missed:
					fDeltaLife = +0.000f;
					break;
				default:
					FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
			}
			break;
		case DrainType_SuddenDeath:
			switch (score) {
				case HNS_Held:
					fDeltaLife = +0;
					break;
				case HNS_LetGo:
					fDeltaLife = -1.0f;
					break;
				case HNS_Missed:
					fDeltaLife = +0;
					break;
				default:
					FAIL_M(ssprintf("Invalid HoldNoteScore: %i", score));
			}
			break;
		default:
			FAIL_M(ssprintf("Invalid DrainType: %i", dtype));
	}

	ChangeLife(fDeltaLife);
}

void
LifeMeterBar::ChangeLife(float fDeltaLife)
{
	// If we've already failed, there's no point in letting them fill up the bar
	// again.
	if (m_pPlayerStageStats->m_bFailed)
		return;

	switch (m_pPlayerState->m_PlayerOptions.GetSong().m_DrainType) {
		case DrainType_Normal:
		case DrainType_NoRecover:
			if (fDeltaLife > 0)
				fDeltaLife *= m_fLifeDifficulty;
			else
				fDeltaLife /= m_fLifeDifficulty;
			break;
		case DrainType_SuddenDeath:
			// This should always -1.0f;
			if (fDeltaLife < 0)
				fDeltaLife = -1.0f;
			else
				fDeltaLife = 0;
			break;
		default:
			break;
	}

	const auto InitialLife = m_fLifePercentage;

	m_fLifePercentage += fDeltaLife;

	// Default theme metrics states that LifeMultiplier is used ONLY for debug.
	// Clearly not true. -Mina
	CLAMP(m_fLifePercentage, 0, LIFE_MULTIPLIER);

	// Only send life changed messages if the life value has actually changed.
	// -Mina
	if (InitialLife != m_fLifePercentage)
		AfterLifeChanged();
}

void
LifeMeterBar::SetLife(float value)
{
	m_fLifePercentage = value;
	CLAMP(m_fLifePercentage, 0, LIFE_MULTIPLIER);
	AfterLifeChanged();
}

extern ThemeMetric<bool> PENALIZE_TAP_SCORE_NONE;
void
LifeMeterBar::HandleTapScoreNone()
{
	if (PENALIZE_TAP_SCORE_NONE)
		ChangeLife(TNS_None);
}

void
LifeMeterBar::AfterLifeChanged()
{
	m_pStream->SetPercent(m_fLifePercentage);

	Message msg("LifeChanged");
	msg.SetParam("Player", m_pPlayerState->m_PlayerNumber);
	msg.SetParam("LifeMeter", LuaReference::CreateFromPush(*this));
	MESSAGEMAN->Broadcast(msg);
}

bool
LifeMeterBar::IsHot() const
{
	return m_fLifePercentage >= HOT_VALUE;
}

bool
LifeMeterBar::IsInDanger() const
{
	return m_fLifePercentage < DANGER_THRESHOLD;
}

bool
LifeMeterBar::IsFailing() const
{
	return m_fLifePercentage <=
		   m_pPlayerState->m_PlayerOptions.GetCurrent().m_fPassmark;
}

void
LifeMeterBar::Update(float fDeltaTime)
{
	LifeMeter::Update(fDeltaTime);

	m_fPassingAlpha += !IsFailing() ? +fDeltaTime * 2 : -fDeltaTime * 2;
	CLAMP(m_fPassingAlpha, 0, 1);

	m_fHotAlpha += IsHot() ? +fDeltaTime * 2 : -fDeltaTime * 2;
	CLAMP(m_fHotAlpha, 0, 1);

	m_pStream->SetPassingAlpha(m_fPassingAlpha);
	m_pStream->SetHotAlpha(m_fHotAlpha);

	if (m_pPlayerState->m_HealthState == HealthState_Danger)
		m_sprDanger->SetVisible(true);
	else
		m_sprDanger->SetVisible(false);
}

void
LifeMeterBar::FillForHowToPlay(int NumW2s, int NumMisses)
{
	m_iProgressiveLifebar = 0; // disable progressive lifebar

	const auto AmountForW2 = NumW2s * m_fLifeDifficulty * 0.008f;
	const auto AmountForMiss = NumMisses / m_fLifeDifficulty * 0.08f;

	m_fLifePercentage = AmountForMiss - AmountForW2;
	CLAMP(m_fLifePercentage, 0.0f, 1.0f);
	AfterLifeChanged();
}
