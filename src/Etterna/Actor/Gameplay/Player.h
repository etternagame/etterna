#ifndef PLAYER_H
#define PLAYER_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Screen/Others/ScreenMessage.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include <chrono>

class LifeMeter;
class ScoreKeeper;
class RageTimer;
class NoteField;
class PlayerStageStats;
class JudgedRows;
class PlayerState;
class HoldJudgment;

// todo: replace these with a Message and MESSAGEMAN? -aj
AutoScreenMessage(SM_100Combo);
AutoScreenMessage(SM_200Combo);
AutoScreenMessage(SM_300Combo);
AutoScreenMessage(SM_400Combo);
AutoScreenMessage(SM_500Combo);
AutoScreenMessage(SM_600Combo);
AutoScreenMessage(SM_700Combo);
AutoScreenMessage(SM_800Combo);
AutoScreenMessage(SM_900Combo);
AutoScreenMessage(SM_1000Combo);
AutoScreenMessage(SM_ComboStopped);
AutoScreenMessage(SM_ComboContinuing);

/** @brief Accepts input, knocks down TapNotes that were stepped on, and keeps
 * score for the player. */
class Player : public ActorFrame
{
  public:
	// The passed in NoteData isn't touched until Load() is called.
	Player(NoteData& nd, bool bVisibleParts = true);
	~Player() override;

	void Update(float fDeltaTime) override;
	void DrawPrimitives() override;
	// PushPlayerMatrix and PopPlayerMatrix are separate functions because
	// they need to be used twice so that the notefield board can rendered
	// underneath the combo and judgment.  They're not embedded in
	// PlayerMatrixPusher so that some nutjob can later decide to expose them
	// to lua. -Kyz
	void PushPlayerMatrix(float x, float skew, float center_y);
	void PopPlayerMatrix();

	// This exists so that the board can be drawn underneath combo/judge. -Kyz
	void DrawNoteFieldBoard();

	// Here's a fun construct for people that haven't seen it before:
	// This object does some task when it's created, then cleans up when it's
	// destroyed.  That way, you stick it inside a block, and can't forget the
	// cleanup. -Kyz
	struct PlayerNoteFieldPositioner
	{
		PlayerNoteFieldPositioner(Player* p,
								  float x,
								  float tilt,
								  float skew,
								  float mini,
								  float center_y,
								  bool reverse);
		~PlayerNoteFieldPositioner();
		Player* player;
		float original_y;
		float y_offset;
	};

	struct TrackRowTapNote
	{
		int iTrack;
		int iRow;
		TapNote* pTN;
	};
	virtual void UpdateHoldNotes(int iSongRow,
								 float fDeltaTime,
                                 std::vector<TrackRowTapNote>& vTN);

	virtual void Init(const std::string& sType,
					  PlayerState* pPlayerState,
					  PlayerStageStats* pPlayerStageStats,
					  LifeMeter* pLM,
					  ScoreKeeper* pPrimaryScoreKeeper);
	virtual void Load();
	virtual void Reload();
	virtual void CrossedRows(int iLastRowCrossed,
							 const std::chrono::steady_clock::time_point& now);
	/**
	 * @brief Retrieve the Player's TimingData.
	 *
	 * This is primarily for a lua hook.
	 * @return the TimingData in question. */
	TimingData GetPlayerTimingData() const { return *(this->m_Timing); }

	void ScoreAllActiveHoldsLetGo();
	void DoTapScoreNone();
	void AddHoldToReplayData(int col,
							 const TapNote* pTN,
							 int RowOfOverlappingNoteOrRow);
	void AddNoteToReplayData(int col,
							 const TapNote* pTN,
							 int RowOfOverlappingNoteOrRow);

	virtual void Step(int col,
					  int row,
					  const std::chrono::steady_clock::time_point& tm,
					  bool bHeld,
					  bool bRelease,
					  float padStickSeconds = 0.0f);

	void FadeToFail();
	void CacheAllUsedNoteSkins();
	TapNoteScore GetLastTapNoteScore() const { return m_LastTapNoteScore; }
	void SetPaused(bool bPaused) { m_bPaused = bPaused; }

	static float GetMaxStepDistanceSeconds();
	static float GetWindowSeconds(TimingWindow tw);
	static float GetWindowSecondsCustomScale(TimingWindow tw,
											 float timingScale = 1.f);
	static float GetTimingWindowScale();
	const NoteData& GetNoteData() const { return m_NoteData; }
	bool HasVisibleParts() const { return m_pNoteField != NULL; }

	void SetActorWithJudgmentPosition(Actor* pActor)
	{
		m_pActorWithJudgmentPosition = pActor;
	}
	void SetActorWithComboPosition(Actor* pActor)
	{
		m_pActorWithComboPosition = pActor;
	}

	void SetSendJudgmentAndComboMessages(bool b)
	{
		m_bSendJudgmentAndComboMessages = b;
	}
	void RenderAllNotesIgnoreScores();

	// Lua
	void PushSelf(lua_State* L) override;

	PlayerState* GetPlayerState() { return this->m_pPlayerState; }
	void ChangeLife(float delta);
	void SetLife(float value);
	bool m_inside_lua_set_life;

	// Mina perma-temp stuff
	std::vector<int> nerv;   // the non empty row vector where we are somehwere in
	size_t nervpos = 0; // where we are in the non-empty row vector
	float maxwifescore = 0.f;
	float curwifescore = 0.f;
	float wifescorepersonalbest = 0.f;
	int totalwifescore;

  protected:
	static bool NeedsTapJudging(const TapNote& tn);
	static bool NeedsHoldJudging(const TapNote& tn);
	void UpdateTapNotesMissedOlderThan(float fMissIfOlderThanThisBeat);
	void UpdateJudgedRows(float fDeltaTime);
	// Updates visible parts: Hold Judgments, NoteField Zoom, Combo based Actors
	void UpdateVisibleParts();
	// Updates the pressed flags depending on input
	// Tells the NoteField to do stuff basically
	void UpdatePressedFlags();
	// Updates Holds and Rolls
	// For Rolls, just tells Autoplay to restep them
	// For Holds, tells their life to decay
	// ... oh man this is redundant
	void UpdateHoldsAndRolls(float fDeltaTime,
							 const std::chrono::steady_clock::time_point& now);
	// Updates Crossed Rows for NoteData
	// What this involves is:
	//		Hold Life/Tapping Heads/Checkpoints
	//		Mines (The act of holding a button to hit one)
	//		Autoplay hitting taps
	//		Keysounds
	void UpdateCrossedRows(const std::chrono::steady_clock::time_point& now);
	void FlashGhostRow(int iRow);
	void HandleTapRowScore(unsigned row);
	void HandleHoldScore(const TapNote& tn);
	void HandleHoldCheckpoint(int iRow,
							  int iNumHoldsHeldThisRow,
							  int iNumHoldsMissedThisRow,
							  const std::vector<int>& viColsWithHold);
	void DrawTapJudgments();
	void DrawHoldJudgments();
	void SendComboMessages(unsigned int iOldCombo, unsigned int iOldMissCombo);
	void PlayKeysound(const TapNote& tn, TapNoteScore score);

	void SetMineJudgment(TapNoteScore tns, int iTrack);
	void SetJudgment(int iRow, int iFirstTrack, const TapNote& tn)
	{
		SetJudgment(
		  iRow, iFirstTrack, tn, tn.result.tns, tn.result.fTapNoteOffset);
	}
	void SetJudgment(int iRow,
					 int iFirstTrack,
					 const TapNote& tn,
					 TapNoteScore tns,
					 float fTapNoteOffset); // -1 if no track as in TNS_Miss
	void SetHoldJudgment(TapNote& tn, int iTrack, int iRow);
	void SetCombo(unsigned int iCombo, unsigned int iMisses);
	void IncrementComboOrMissCombo(bool bComboOrMissCombo);
	void IncrementCombo() { IncrementComboOrMissCombo(true); };
	void IncrementMissCombo() { IncrementComboOrMissCombo(false); };

	void ChangeLife(TapNoteScore tns);
	void ChangeLife(HoldNoteScore hns, TapNoteScore tns);
	void ChangeLifeRecord();

	void ChangeWifeRecord();

	int GetClosestNoteDirectional(int col,
								  int iStartRow,
								  int iMaxRowsAhead,
								  bool bAllowGraded,
								  bool bForward) const;
	int GetClosestNote(int col,
					   int iNoteRow,
					   int iMaxRowsAhead,
					   int iMaxRowsBehind,
					   bool bAllowGraded,
					   bool bAllowOldMines = true) const;
	int GetClosestNonEmptyRowDirectional(int iStartRow,
										 int iMaxRowsAhead,
										 bool bAllowGraded,
										 bool bForward) const;
	int GetClosestNonEmptyRow(int iNoteRow,
							  int iMaxRowsAhead,
							  int iMaxRowsBehind,
							  bool bAllowGraded) const;

	inline void HideNote(int col, int row)
	{
		NoteData::iterator iter = m_NoteData.FindTapNote(col, row);
		if (iter != m_NoteData.end(col))
			iter->second.result.bHidden = true;
	}

	bool m_bLoaded;

	/** @brief The player's present state. */
	PlayerState* m_pPlayerState;
	/** @brief The player's present stage stats. */
	PlayerStageStats* m_pPlayerStageStats;
	TimingData* m_Timing;
	float m_fNoteFieldHeight;

	std::vector<float> lastHoldHeadsSeconds;

	bool m_bPaused;
	bool m_bDelay;

	NoteData& m_NoteData;
	NoteField* m_pNoteField;

	std::vector<HoldJudgment*> m_vpHoldJudgment;

	AutoActor m_sprJudgment;
	AutoActor m_sprCombo;
	Actor* m_pActorWithJudgmentPosition;
	Actor* m_pActorWithComboPosition;

	TapNoteScore m_LastTapNoteScore;
	LifeMeter* m_pLifeMeter;
	ScoreKeeper* m_pPrimaryScoreKeeper;

	int m_iFirstUncrossedRow; // used by hold checkpoints logic
	NoteData::all_tracks_iterator* m_pIterNeedsTapJudging;
	NoteData::all_tracks_iterator* m_pIterNeedsHoldJudging;
	NoteData::all_tracks_iterator* m_pIterUncrossedRows;
	NoteData::all_tracks_iterator* m_pIterUnjudgedRows;
	NoteData::all_tracks_iterator* m_pIterUnjudgedMineRows;
	unsigned int m_iLastSeenCombo;
	bool m_bSeenComboYet;
	JudgedRows* m_pJudgedRows;

	RageSound m_soundMine;

	std::vector<RageSound> m_vKeysounds;

	ThemeMetric<float> GRAY_ARROWS_Y_STANDARD;
	ThemeMetric<float> GRAY_ARROWS_Y_REVERSE;
	ThemeMetric<float> HOLD_JUDGMENT_Y_STANDARD;
	ThemeMetric<float> HOLD_JUDGMENT_Y_REVERSE;
	ThemeMetric<int> BRIGHT_GHOST_COMBO_THRESHOLD;
	ThemeMetric<bool> TAP_JUDGMENTS_UNDER_FIELD;
	ThemeMetric<bool> HOLD_JUDGMENTS_UNDER_FIELD;
	ThemeMetric<bool> COMBO_UNDER_FIELD;
	ThemeMetric<int> DRAW_DISTANCE_AFTER_TARGET_PIXELS;
	ThemeMetric<int> DRAW_DISTANCE_BEFORE_TARGET_PIXELS;
	/**
	  Does repeatedly stepping on a roll to keep it alive increment the
	  combo?

	  If set to true, repeatedly stepping on a roll will increment the combo.
	  If set to false, only the roll head causes the combo to be incremented.

	  For those wishing to make a theme very accurate to In The Groove 2, set
	  this to false.
	  PLAYER INIT MUST LOAD THIS OR YOU CRASH
	  */
	ThemeMetric<bool> ROLL_BODY_INCREMENTS_COMBO;
	ThemeMetric<bool> COMBO_BREAK_ON_IMMEDIATE_HOLD_LET_GO;

#define NUM_REVERSE 2
#define NUM_CENTERED 2
	TweenState m_tsJudgment[NUM_REVERSE][NUM_CENTERED];
	TweenState m_tsCombo[NUM_REVERSE][NUM_CENTERED];

	bool m_bSendJudgmentAndComboMessages;
	bool m_bTickHolds;
	// This exists so that the board can be drawn underneath combo/judge. -Kyz
	bool m_drawing_notefield_board;
};

/**
 * @brief Helper class to ensure that each row is only judged once without
 * taking too much memory.
 */
class JudgedRows
{
    std::vector<bool> m_vRows;
	int m_iStart{ 0 };
	int m_iOffset{ 0 };

	void Resize(size_t iMin)
	{
		size_t iNewSize = std::max(2 * m_vRows.size(), iMin);
        std::vector<bool> vNewRows(m_vRows.begin() + m_iOffset, m_vRows.end());
		vNewRows.reserve(iNewSize);
		vNewRows.insert(
		  vNewRows.end(), m_vRows.begin(), m_vRows.begin() + m_iOffset);
		vNewRows.resize(iNewSize, false);
		m_vRows.swap(vNewRows);
		m_iOffset = 0;
	}

  public:
	JudgedRows() { Resize(32); }
	// Returns true if the row has already been judged.
	bool JudgeRow(int iRow)
	{
		if (iRow < m_iStart)
			return true;
		if (iRow >= m_iStart + static_cast<int>(m_vRows.size()))
			Resize(iRow + 1 - m_iStart);
		const int iIndex = (iRow - m_iStart + m_iOffset) % m_vRows.size();
		const bool ret = m_vRows[iIndex];
		m_vRows[iIndex] = true;
		while (m_vRows[m_iOffset]) {
			m_vRows[m_iOffset] = false;
			++m_iStart;
			if (++m_iOffset >= static_cast<int>(m_vRows.size()))
				m_iOffset -= m_vRows.size();
		}
		return ret;
	}
	void Reset(int iStart)
	{
		m_iStart = iStart;
		m_iOffset = 0;
		m_vRows.assign(m_vRows.size(), false);
	}
};

#endif
