#ifndef PLAYER_H
#define PLAYER_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Screen/Others/ScreenMessage.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/Misc/TimingData.h"

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

constexpr float initialHoldLife = 1.F;

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
	static void PushPlayerMatrix(float x, float skew, float center_y);
	static void PopPlayerMatrix();

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
	auto GetPlayerTimingData() const -> TimingData { return *(this->m_Timing); }

	void ScoreAllActiveHoldsLetGo();
	void DoTapScoreNone();
	void AddHoldToReplayData(int col,
							 const TapNote* pTN,
							 int RowOfOverlappingNoteOrRow) const;
	void AddNoteToReplayData(int col,
							 const TapNote* pTN,
							 int RowOfOverlappingNoteOrRow) const;
	void AddMineToReplayData(int col, int row) const;

	virtual void Step(int col,
					  int row,
					  const std::chrono::steady_clock::time_point& tm,
					  bool bHeld,
					  bool bRelease,
					  float padStickSeconds = 0.0F);

	void FadeToFail();
	void CacheAllUsedNoteSkins() const;
	auto GetLastTapNoteScore() const -> TapNoteScore
	{
		return m_LastTapNoteScore;
	}
	void SetPaused(bool bPaused) { m_bPaused = bPaused; }

	static auto GetMaxStepDistanceSeconds() -> float;
	static auto GetWindowSeconds(TimingWindow tw) -> float;
	static auto GetWindowSecondsCustomScale(TimingWindow tw,
											float timingScale = 1.F) -> float;
	static auto GetTimingWindowScale() -> float;
	auto GetNoteData() const -> const NoteData& { return m_NoteData; }
	auto HasVisibleParts() const -> bool { return m_pNoteField != nullptr; }

	void SetSendJudgmentAndComboMessages(bool b)
	{
		m_bSendJudgmentAndComboMessages = b;
	}
	void RenderAllNotesIgnoreScores();

	// Lua
	void PushSelf(lua_State* L) override;

	auto GetPlayerState() const -> PlayerState* { return this->m_pPlayerState; }
	void ChangeLife(float delta) const;
	void SetLife(float value) const;
	bool m_inside_lua_set_life;

	// Mina perma-temp stuff
	std::vector<int> nerv;	// the non empty row vector where we are somehwere in
	size_t nervpos = 0; // where we are in the non-empty row vector
	float maxwifescore = 0.F;
	float curwifescore = 0.F;
	float wifescorepersonalbest = 0.F;
	int totalwifescore;

  protected:
	static auto NeedsTapJudging(const TapNote& tn) -> bool;
	static auto NeedsHoldJudging(const TapNote& tn) -> bool;
	virtual void UpdateTapNotesMissedOlderThan(float fMissIfOlderThanSeconds);
	void UpdateJudgedRows(float fDeltaTime);
	// Updates visible parts: Hold Judgments, NoteField Zoom, Combo based Actors
	void UpdateVisibleParts();
	// Updates the pressed flags depending on input
	// Tells the NoteField to do stuff basically
	virtual void UpdatePressedFlags();
	// Updates Holds and Rolls
	// For Rolls, just tells Autoplay to restep them
	// For Holds, tells their life to decay
	// ... oh man this is redundant
	virtual void UpdateHoldsAndRolls(
	  float fDeltaTime,
	  const std::chrono::steady_clock::time_point& now);
	// Updates Crossed Rows for NoteData
	// What this involves is:
	//		Hold Life/Tapping Heads/Checkpoints
	//		Mines (The act of holding a button to hit one)
	//		Autoplay hitting taps
	//		Keysounds
	void UpdateCrossedRows(const std::chrono::steady_clock::time_point& now);
	void FlashGhostRow(int iRow);
	virtual void HandleTapRowScore(unsigned row);
	void HandleHoldScore(const TapNote& tn) const;
	void HandleHoldCheckpoint(int iRow,
							  int iNumHoldsHeldThisRow,
							  int iNumHoldsMissedThisRow,
							  const std::vector<int>& viColsWithHold);
	void DrawTapJudgments();
	void DrawHoldJudgments();
	void SendComboMessages(unsigned int iOldCombo,
						   unsigned int iOldMissCombo) const;
	void PlayKeysound(const TapNote& tn, TapNoteScore score);

	void SetMineJudgment(TapNoteScore tns, int iTrack, int iRow);
	void SetJudgment(int iRow, int iFirstTrack, const TapNote& tn)
	{
		SetJudgment(
		  iRow, iFirstTrack, tn, tn.result.tns, tn.result.fTapNoteOffset);
	}
	void SetJudgment(int iRow,
					 int iTrack,
					 const TapNote& tn,
					 TapNoteScore tns,
					 float fTapNoteOffset); // -1 if no track as in TNS_Miss
	void SetHoldJudgment(TapNote& tn, int iTrack, int iRow);
	void SetCombo(unsigned int iCombo, unsigned int iMisses);
	void IncrementComboOrMissCombo(bool bComboOrMissCombo);
	void IncrementCombo() { IncrementComboOrMissCombo(true); };
	void IncrementMissCombo() { IncrementComboOrMissCombo(false); };

	void ChangeLife(TapNoteScore tns) const;
	void ChangeLife(HoldNoteScore hns, TapNoteScore tns) const;
	void ChangeLifeRecord() const;

	void ChangeWifeRecord() const;

	auto GetClosestNoteDirectional(int col,
								   int iStartRow,
								   int iEndRow,
								   bool bAllowGraded,
								   bool bForward) const -> int;
	auto GetClosestNote(int col,
						int iNoteRow,
						int iMaxRowsAhead,
						int iMaxRowsBehind,
						bool bAllowGraded,
						bool bUseSongTiming = true,
						bool bAllowOldMines = true) const -> int;
	auto GetClosestNonEmptyRowDirectional(int iStartRow,
										  int iEndRow,
										  bool bAllowGraded,
										  bool bForward) const -> int;
	auto GetClosestNonEmptyRow(int iNoteRow,
							   int iMaxRowsAhead,
							   int iMaxRowsBehind,
							   bool bAllowGraded) const -> int;

	void HideNote(int col, int row) const
	{
		const auto iter = m_NoteData.FindTapNote(col, row);
		if (iter != m_NoteData.end(col)) {
			iter->second.result.bHidden = true;
		}
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
	void Resize(size_t iMin);

  public:
	JudgedRows() { Resize(32); }
	// Returns true if the row has already been judged.
	auto JudgeRow(int iRow) -> bool
	{
		if (iRow < m_iStart) {
			return true;
		}
		if (iRow >= m_iStart + static_cast<int>(m_vRows.size())) {
			Resize(iRow + 1 - m_iStart);
		}
		const int iIndex = (iRow - m_iStart + m_iOffset) % m_vRows.size();
		const bool ret = m_vRows[iIndex];
		m_vRows[iIndex] = true;
		while (m_vRows[m_iOffset]) {
			m_vRows[m_iOffset] = false;
			++m_iStart;
			if (++m_iOffset >= static_cast<int>(m_vRows.size())) {
				m_iOffset -= m_vRows.size();
			}
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
