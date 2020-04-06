#ifndef LIFEMETERBAR_H
#define LIFEMETERBAR_H

#include "Etterna/Actor/Base/AutoActor.h"
#include "LifeMeter.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class StreamDisplay;

/** @brief The player's life represented as a bar. */
class LifeMeterBar : public LifeMeter
{
  public:
	LifeMeterBar();
	~LifeMeterBar() override;

	void Load(const PlayerState* pPlayerState,
			  PlayerStageStats* pPlayerStageStats) override;

	void Update(float fDeltaTime) override;

	void ChangeLife(TapNoteScore score) override;
	void ChangeLife(HoldNoteScore score, TapNoteScore tscore) override;
	void ChangeLife(float fDeltaLifePercent) override;
	void SetLife(float value) override;
	void HandleTapScoreNone() override;
	virtual void AfterLifeChanged();
	bool IsInDanger() const override;
	bool IsHot() const override;
	bool IsFailing() const override;
	float GetLife() const override { return m_fLifePercentage; }

	void FillForHowToPlay(int NumT2s, int NumMisses);
	static float MapTNSToDeltaLife(TapNoteScore s);
	static float MapHNSToDeltaLife(HoldNoteScore score);

  private:
	ThemeMetric<float> DANGER_THRESHOLD;
	ThemeMetric<float> INITIAL_VALUE;
	ThemeMetric<float> HOT_VALUE;
	ThemeMetric<float> LIFE_MULTIPLIER;
	ThemeMetric<TapNoteScore> MIN_STAY_ALIVE;

	ThemeMetric1D<float> m_fLifePercentChange;

	// Doing this proper, let's not vector lookup for these values every update
	// - Mina
	float m_Change_SE_W1;
	float m_Change_SE_W2;
	float m_Change_SE_W3;
	float m_Change_SE_W4;
	float m_Change_SE_W5;
	float m_Change_SE_Miss;
	float m_Change_SE_HitMine;
	float m_Change_SE_CheckpointHit;
	float m_Change_SE_CheckpointMiss;
	float m_Change_SE_Held;
	float m_Change_SE_LetGo;

	AutoActor m_sprUnder;
	AutoActor m_sprDanger;
	StreamDisplay* m_pStream;
	AutoActor m_sprOver;

	float m_fLifePercentage;

	float m_fPassingAlpha;
	float m_fHotAlpha;

	float m_fBaseLifeDifficulty;
	float m_fLifeDifficulty; // essentially same as pref

	int m_iProgressiveLifebar; // cached from prefs
	/** @brief The current number of progressive W5/miss rankings. */
	int m_iMissCombo;
	/** @brief The combo needed before the life bar starts to fill up after a
	 * Player failed. */
	int m_iComboToRegainLife;
};

#endif
