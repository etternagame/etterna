/* ScoreKeeperNormal -  */

#ifndef SCORE_KEEPER_NORMAL_H
#define SCORE_KEEPER_NORMAL_H

#include "ScoreKeeper.h"
#include "Etterna/Screen/Others/ScreenMessage.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
class Steps;
class Song;
struct RadarValues;
class TimingData;

AutoScreenMessage(SM_PlayToasty);

/** @brief The default ScoreKeeper implementation. */
class ScoreKeeperNormal : public ScoreKeeper
{
	void AddScoreInternal(TapNoteScore score);
	auto CalcNextToastyAt(int level) -> int;

	int m_iScoreRemainder;
	int m_iMaxPossiblePoints;
	int m_iTapNotesHit; // number of notes judged so far, needed by scoring

	int m_iNumTapsAndHolds;
	int m_iMaxScoreSoFar; // for nonstop scoring
	int m_iPointBonus;	// the difference to award at the end
	int m_cur_toasty_combo;
	int m_cur_toasty_level;
	int m_next_toasty_at;
	bool m_bIsLastSongInCourse;
	bool m_bIsBeginner;

	int m_iNumNotesHitThisRow; // Used by Custom Scoring only

	ThemeMetric<bool> m_ComboIsPerRow;
	ThemeMetric<bool> m_MissComboIsPerRow;
	ThemeMetric<TapNoteScore> m_MinScoreToContinueCombo;
	ThemeMetric<TapNoteScore> m_MinScoreToMaintainCombo;
	ThemeMetric<TapNoteScore> m_MaxScoreToIncrementMissCombo;
	ThemeMetric<bool> m_MineHitIncrementsMissCombo;
	ThemeMetric<bool> m_AvoidMineIncrementsCombo;
	ThemeMetric<bool> m_UseInternalScoring;

	ThemeMetric<TapNoteScore> m_toasty_min_tns;
	ThemeMetric<LuaReference> m_toasty_trigger;

	vector<Steps*> m_apSteps;

	virtual void AddTapScore(TapNoteScore tns);
	virtual void AddHoldScore(HoldNoteScore hns);
	virtual void AddTapRowScore(TapNoteScore tns, const NoteData& nd, int iRow);

	/* Configuration: */
	/* Score after each tap will be rounded to the nearest m_lroundTo; 1 to do
	 * nothing. */
	int m_lroundTo;
	int m_ComboBonusFactor[NUM_TapNoteScore];

  public:
	ScoreKeeperNormal(PlayerState* pPlayerState,
					  PlayerStageStats* pPlayerStageStats);

	void Load(const vector<Song*>& apSongs,
			  const vector<Steps*>& apSteps) override;

	// before a song plays (called multiple times if course)
	void OnNextSong(int iSongInCourseIndex,
					const Steps* pSteps,
					const NoteData* pNoteData) override;

	void HandleTapScore(const TapNote& tn) override;
	void HandleTapRowScore(const NoteData& nd, int iRow) override;
	void HandleHoldScore(const TapNote& tn) override;
	void HandleHoldActiveSeconds(float /* fMusicSecondsHeld */) override{};
	void HandleHoldCheckpointScore(const NoteData& nd,
								   int iRow,
								   int iNumHoldsHeldThisRow,
								   int iNumHoldsMissedThisRow) override;
	void HandleTapScoreNone() override;

	// This must be calculated using only cached radar values so that we can
	// do it quickly.
	static auto GetPossibleDancePoints(NoteData* nd,
									   const TimingData* td) -> int;
	static auto GetPossibleDancePoints(NoteData* ndPre,
									   NoteData* ndPost,
									   const TimingData* td) -> int;
	static auto GetPossibleGradePoints(NoteData* nd,
									   const TimingData* td) -> int;
	static auto GetPossibleGradePoints(NoteData* ndPre,
									   NoteData* ndPost,
									   const TimingData* td) -> int;

	auto TapNoteScoreToDancePoints(TapNoteScore tns) const -> int;
	auto HoldNoteScoreToDancePoints(HoldNoteScore hns) const -> int;
	auto TapNoteScoreToGradePoints(TapNoteScore tns) const -> int;
	auto HoldNoteScoreToGradePoints(HoldNoteScore hns) const -> int;
	static auto TapNoteScoreToDancePoints(TapNoteScore tns, bool bBeginner)
	  -> int;
	static auto HoldNoteScoreToDancePoints(HoldNoteScore hns, bool bBeginner)
	  -> int;
	static auto TapNoteScoreToGradePoints(TapNoteScore tns, bool bBeginner)
	  -> int;
	static auto HoldNoteScoreToGradePoints(HoldNoteScore hns, bool bBeginner)
	  -> int;

  private:
	/**
	 * @brief Take care of some internal work with our scoring systems.
	 * @param tns the Tap Note score earned.
	 * @param maximum the best tap note score possible.
	 * @param row the row the score was earned. Mainly for ComboSegment stuff.
	 */
	void HandleTapNoteScoreInternal(const NoteData& nd,
									TapNoteScore tns,
									TapNoteScore maximum,
									int row,
									bool separately);
	void HandleComboInternal(int iNumHitContinueCombo,
							 int iNumHitMaintainCombo,
							 int iNumBreakCombo,
							 int iRow = -1);
	void HandleRowComboInternal(TapNoteScore tns,
								int iNumTapsInRow,
								int iRow = -1);
	void GetRowCounts(const NoteData& nd,
					  int iRow,
					  int& iNumHitContinueCombo,
					  int& iNumHitMaintainCombo,
					  int& iNumBreakCombo);
};

#endif
