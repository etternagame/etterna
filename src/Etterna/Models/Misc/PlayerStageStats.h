#ifndef PlayerStageStats_H
#define PlayerStageStats_H

#include "Grade.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "PlayerNumber.h"
#include "RadarValues.h"
#include "NoteTypes.h"
#include <map>

class Steps;
class Style;
struct lua_State;

/** @brief Contains statistics for one stage of play - either one song, or a
 * whole course. */
class PlayerStageStats
{
  public:
	/** @brief Set up the PlayerStageStats with default values. */
	PlayerStageStats() { InternalInit(); }
	void InternalInit();
	void Init(PlayerNumber pn);
	void Init(MultiPlayer pn);

	std::vector<NoteInfo> serializednd;

	/**
	 * @brief Add stats from one PlayerStageStats to another.
	 * @param other the other stats to add to this one. */
	void AddStats(const PlayerStageStats& other); // accumulate

	static auto GetGrade(float p) -> Grade;
	[[nodiscard]] auto GetGrade() const -> Grade;
	static auto MakePercentScore(int iActual, int iPossible) -> float;
	static auto FormatPercentScore(float fPercentScore) -> std::string;
	// Calculate the difficulty rating for a specific score obtained by a player
	// - Mina
	auto GetWifeGrade() -> Grade;
	[[nodiscard]] auto CalcSSR(float ssrpercent) const -> std::vector<float>;
	void GenerateValidationKeys(HighScore& hs) const;
	[[nodiscard]] auto GetPercentDancePoints() const -> float;
	[[nodiscard]] auto GetWifeScore() const -> float;
	[[nodiscard]] auto GetCurWifeScore() const -> float;
	[[nodiscard]] auto GetMaxWifeScore() const -> float;
	[[nodiscard]] auto GetTimingScale() const -> float;
	[[nodiscard]] auto GetInputDataVector() const -> std::vector<InputDataEvent>;
	[[nodiscard]] auto GetMissDataVector() const
	  -> std::vector<MissReplayResult>;
	[[nodiscard]] auto GetOffsetVector() const -> std::vector<float>;
	[[nodiscard]] auto GetNoteRowVector() const -> std::vector<int>;
	[[nodiscard]] auto GetTrackVector() const -> std::vector<int>;
	[[nodiscard]] auto GetTapNoteTypeVector() const -> std::vector<TapNoteType>;
	[[nodiscard]] auto GetHoldReplayDataVector() const
	  -> std::vector<HoldReplayResult>;
	[[nodiscard]] auto GetMineReplayDataVector() const
	  -> std::vector<MineReplayResult>;
	[[nodiscard]] auto GetCurMaxPercentDancePoints() const -> float;

	[[nodiscard]] auto GetLessonScoreActual() const -> int;
	[[nodiscard]] auto GetLessonScoreNeeded() const -> int;
	void ResetScoreForLesson();

	bool m_for_multiplayer{};
	PlayerNumber m_player_number;
	MultiPlayer m_multiplayer_number;
	const Style* m_pStyle{};

	bool m_bJoined{};
	bool m_bPlayerCanAchieveFullCombo{};
	std::vector<Steps*> m_vpPossibleSteps;
	int m_iStepsPlayed{}; // how many of m_vpPossibleStepshow many of
						  // m_vpPossibleSteps were played

	/**
	 * @brief Have the Players failed at any point during the song?
	 *
	 * If FAIL_OFF is in use, this is always false.
	 *
	 * If health recovery is possible after failing (requires two players),
	 * this is only set if both players were failing at the same time. */
	bool m_bFailed{};

	int m_iPossibleDancePoints{};
	int m_iCurPossibleDancePoints{};
	int m_iActualDancePoints{};
	int m_iPossibleGradePoints{};
	float m_fWifeScore{};
	float CurWifeScore{};
	float MaxWifeScore{};
	float m_fTimingScale{};
	std::vector<MineReplayResult> m_vMineReplayData;
	std::vector<HoldReplayResult> m_vHoldReplayData;
	std::vector<float> m_vOffsetVector;
	std::vector<int> m_vNoteRowVector;
	std::vector<TapNoteType> m_vTapNoteTypeVector;
	std::vector<int> m_vTrackVector;
	std::vector<InputDataEvent> InputData;
	std::vector<MissReplayResult> m_vNoteMissVector;
	int m_iTapNoteScores[NUM_TapNoteScore]{};
	int m_iHoldNoteScores[NUM_HoldNoteScore]{};
	/** @brief The Player's current combo. */
	unsigned int m_iCurCombo{};
	/** @brief The Player's max combo. */
	unsigned int m_iMaxCombo{};
	/** @brief The Player's current miss combo. */
	unsigned int m_iCurMissCombo{};
	int m_iCurScoreMultiplier{};
	/** @brief The player's current score. */
	unsigned int m_iScore{};
	/** @brief The theoretically highest score the Player could have at this
	 * point. */
	unsigned int m_iCurMaxScore{};
	/** @brief The maximum score the Player can get this goaround. */
	unsigned int m_iMaxScore{};

	/**
	 * @brief The possible RadarValues for a song.
	 *
	 * This is filled in by ScreenGameplay on the start of the notes. */
	RadarValues m_radarPossible;
	RadarValues m_radarActual;
	/** @brief How many songs were passed by the Player? */
	int m_iSongsPassed{};
	/** @brief How many songs were played by the Player? */
	int m_iSongsPlayed{};
	/**
	 * @brief How many seconds were left for the Player?
	 *
	 * This is used in the Survival mode. */
	float m_fLifeRemainingSeconds{};

	// workout
	float m_iNumControllerSteps{};

	bool everusedautoplay{};
	bool luascriptwasloaded{};
	bool filehadnegbpms{}; // the call after gameplay is over is apparently
						   // unreliable -mina
	bool filegotmines{}; // this needs to be set before any notedata transforms
	bool filegotholds{};
	bool gaveuplikeadumbass{}; // flag 'giving up' status so i can flag it as
							   // failing so i dont have to remove the feature
							   // entirely -mina
	bool usedDoubleSetup{};

	std::map<float, float> m_fLifeRecord;
	void SetLifeRecordAt(float fLife, float fStepsSecond);
	void GetLifeRecord(float* fLifeOut,
					   int iNumSamples,
					   float fStepsEndSecond) const;
	[[nodiscard]] auto GetLifeRecordAt(float fStepsSecond) const -> float;
	[[nodiscard]] auto GetLifeRecordLerpAt(float fStepsSecond) const -> float;
	[[nodiscard]] auto GetCurrentLife() const -> float;

	std::map<float, float> WifeRecord;
	void SetWifeRecordAt(float Wife, float fStepsSecond);
	void GetWifeRecord(float* WifeOut,
					   int iNumSamples,
					   float fStepsEndSecond) const;
	[[nodiscard]] auto GetWifeRecordAt(float fStepsSecond) const -> float;
	[[nodiscard]] auto GetWifeRecordLerpAt(float fStepsSecond) const -> float;

	struct Combo_t
	{
		// Update GetComboList in PlayerStageStats.cpp when adding new members
		// that should be visible from the Lua side.
		/**
		 * @brief The start time of the combo.
		 *
		 * This uses the same scale as the combo list mapping. */
		float m_fStartSecond{ 0 };
		/**
		 * @brief The size time of the combo.
		 *
		 * This uses the same scale as the life record. */
		float m_fSizeSeconds{ 0 };

		/** @brief The size of the Combo, in steps. */
		int m_cnt{ 0 };

		/**
		 * @brief The size of the combo that didn't come from this stage.
		 *
		 * This is generally rolled over from the last song.
		 * It is also a subset of m_cnt. */
		int m_rollover{ 0 };

		/**
		 * @brief Retrieve the size of the combo that came from this song.
		 * @return this song's combo size. */
		[[nodiscard]] auto GetStageCnt() const -> int
		{
			return m_cnt - m_rollover;
		}

		Combo_t() = default;
		[[nodiscard]] auto IsZero() const -> bool { return m_fStartSecond < 0; }
	};
	std::vector<Combo_t> m_ComboList;
	float m_fFirstSecond{};
	float m_fLastSecond{};

	[[nodiscard]] auto GetComboAtStartOfStage() const -> int;
	[[nodiscard]] auto FullComboOfScore(TapNoteScore tnsAllGreaterOrEqual) const
	  -> bool;
	[[nodiscard]] auto FullCombo() const -> bool
	{
		return FullComboOfScore(TNS_W3);
	}
	[[nodiscard]] auto GetBestFullComboTapNoteScore() const -> TapNoteScore;
	[[nodiscard]] auto SingleDigitsOfScore(
	  TapNoteScore tnsAllGreaterOrEqual) const -> bool;
	[[nodiscard]] auto OneOfScore(TapNoteScore tnsAllGreaterOrEqual) const
	  -> bool;
	[[nodiscard]] auto GetTotalTaps() const -> int;
	[[nodiscard]] auto GetPercentageOfTaps(TapNoteScore tns) const -> float;
	void UpdateComboList(float fSecond, bool rollover);
	[[nodiscard]] auto GetMaxCombo() const -> Combo_t;

	int m_iPersonalHighScoreIndex{};
	int m_iMachineHighScoreIndex{};
	bool m_bDisqualified{};
	[[nodiscard]] auto IsDisqualified() const -> bool;

	void UnloadReplayData(); // i don't really trust the deconstructors here,
							 // also prefer flexibility in this -mina
	HighScore m_HighScore;
	float m_fPlayedSeconds{};

	// Lua
	void PushSelf(lua_State* L);
};

#endif
