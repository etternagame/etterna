#ifndef REPLAY_H
#define REPLAY_H

#include "Etterna/Models/Misc/EnumHelper.h"
#include "ReplayConstantsAndTypes.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include <set>

struct HighScore;
class TimingData;
class Steps;
class Style;

class Replay
{
  public:
	Replay();
	Replay(HighScore* hs);
	~Replay();

	inline auto GetBasicPath() const -> const std::string
	{
		return BASIC_REPLAY_DIR + scoreKey;
	}

	inline auto GetFullPath() const -> const std::string
	{
		return FULL_REPLAY_DIR + scoreKey;
	}

	inline auto GetInputPath() const -> const std::string
	{
		return INPUT_DATA_DIR + scoreKey;
	}

	auto GetOffsetVector() const -> const std::vector<float>&
	{
		return vOffsetVector;
	}
	auto GetCopyOfOffsetVector() const -> std::vector<float>
	{
		return vOffsetVector;
	}
	void SetOffsetVector(const std::vector<float>& v) { vOffsetVector = v; }

	auto GetNoteRowVector() const -> const std::vector<int>&
	{
		return vNoteRowVector;
	}
	auto GetCopyOfNoteRowVector() const -> std::vector<int>
	{
		return vNoteRowVector;
	}
	void SetNoteRowVector(const std::vector<int>& v) { vNoteRowVector = v; }

	auto GetTrackVector() const -> const std::vector<int>&
	{
		return vTrackVector;
	}
	auto GetCopyOfTrackVector() const -> std::vector<int>
	{
		return vTrackVector;
	}
	void SetTrackVector(const std::vector<int>& v) { vTrackVector = v; }

	auto GetTapNoteTypeVector() const -> const std::vector<TapNoteType>&
	{
		return vTapNoteTypeVector;
	}
	auto GetCopyOfTapNoteTypeVector() const -> std::vector<TapNoteType>
	{
		return vTapNoteTypeVector;
	}
	void SetTapNoteTypeVector(const std::vector<TapNoteType>& v)
	{
		vTapNoteTypeVector = v;
	}

	auto GetHoldReplayDataVector() const -> const std::vector<HoldReplayResult>&
	{
		return vHoldReplayDataVector;
	}
	auto GetCopyOfHoldReplayDataVector() const -> std::vector<HoldReplayResult>
	{
		return vHoldReplayDataVector;
	}
	void SetHoldReplayDataVector(const std::vector<HoldReplayResult>& v)
	{
		vHoldReplayDataVector = v;
	}

	auto GetMineReplayDataVector() const -> const std::vector<MineReplayResult>&
	{
		return vMineReplayDataVector;
	}
	auto GetCopyOfMineReplayDataVector() const -> std::vector<MineReplayResult>
	{
		return vMineReplayDataVector;
	}
	void SetMineReplayDataVector(const std::vector<MineReplayResult>& v)
	{
		vMineReplayDataVector = v;
	}

	auto GetOnlineReplayTimestampVector() const -> const std::vector<float>&
	{
		return vOnlineReplayTimestampVector;
	}
	auto GetCopyOfOnlineReplayTimestampVector() const -> std::vector<float>
	{
		return vOnlineReplayTimestampVector;
	}
	void SetOnlineReplayTimestampVector(const std::vector<float>& v)
	{
		vOnlineReplayTimestampVector = v;
	}

	auto GetInputDataVector() const -> const std::vector<InputDataEvent>&
	{
		return InputData;
	}
	auto GetCopyOfInputDataVector() const -> std::vector<InputDataEvent>
	{
		return InputData;
	}
	void SetInputDataVector(const std::vector<InputDataEvent>& v)
	{
		InputData = v;
	}

	auto GetMissReplayDataVector() const -> const std::vector<MissReplayResult>&
	{
		return vMissReplayDataVector;
	}
	auto GetCopyOfMissReplayDataVector() const -> std::vector<MissReplayResult>
	{
		return vMissReplayDataVector;
	}
	void SetMissReplayDataVector(const std::vector<MissReplayResult>& v)
	{
		vMissReplayDataVector = v;
	}

	auto GetReplaySnapshotMap() const -> const std::map<int, ReplaySnapshot>&
	{
		return m_ReplaySnapshotMap;
	}
	auto GetCopyOfReplaySnapshotMap() const -> std::map<int, ReplaySnapshot>
	{
		return m_ReplaySnapshotMap;
	}
	void SetReplaySnapshotMap(const std::map<int, ReplaySnapshot>& m)
	{
		m_ReplaySnapshotMap = m;
	}

	auto GetJudgeInfo() -> JudgeInfo& { return judgeInfo; }
	auto GetCopyOfJudgeInfo() const -> JudgeInfo { return judgeInfo; }
	void SetJudgeInfo(const JudgeInfo& ji) { judgeInfo = ji; }

	auto GetScoreKey() const -> std::string { return scoreKey; }
	void SetScoreKey(std::string& key) { scoreKey = key; }
	auto GetChartKey() const -> std::string { return chartKey; }
	void SetChartKey(std::string& key) { chartKey = key; }
	auto GetMusicRate() const -> float { return fMusicRate; }
	void SetMusicRate(float f) { fMusicRate = f; }
	auto GetSongOffset() const -> float { return fSongOffset; }
	void SetSongOffset(float f) { fSongOffset = f; }
	auto GetGlobalOffset() const -> float { return fGlobalOffset; }
	void SetGlobalOffset(float f) { fGlobalOffset = f; }
	auto GetRngSeed() const -> int { return rngSeed; }
	void SetRngSeed(int seed) { rngSeed = seed; }
	auto GetModifiers() const -> std::string { return mods; }
	void SetModifiers(std::string& modstr) { mods = modstr; }

	void SetUseReprioritizedNoteRows(bool b)
	{
		if (b != useReprioritizedNoterows) {
			if (isOnlineScore()) {
				if (vOnlineNoteRowVector.empty() &&
					GenerateNoterowsFromTimestamps()) {
					// initial backup
					vOnlineOffsetVector = GetCopyOfOffsetVector();
					vOnlineNoteRowVector = GetCopyOfNoteRowVector();
					vOnlineTrackVector = GetCopyOfTrackVector();
					vOnlineTapNoteTypeVector = GetCopyOfTapNoteTypeVector();
				}
			}
			ClearPrimitiveVectors();
			ClearReprioritizedVectors();
		}
		if (generatedInputData) {
			InputData.clear();
			vMissReplayDataVector.clear();
			vHoldReplayDataVector.clear();
			vMineReplayDataVector.clear();
			generatedInputData = false;
		}
		useReprioritizedNoterows = b;
	}
	auto UsingReprioritizedNoteRows() -> bool
	{
		return useReprioritizedNoterows;
	}
	auto GetReprioritizedMissData() const
	  -> const std::vector<MissReplayResult>&
	{
		return vReprioritizedMissData;
	}
	auto GetCopyOfReprioritizedMissData() const -> std::vector<MissReplayResult>
	{
		return vReprioritizedMissData;
	}
	void SetReprioritizedMissData(const std::vector<MissReplayResult>& v) {
		vReprioritizedMissData = v;
	}
	auto GetReprioritizedHoldData() const
	  -> const std::vector<HoldReplayResult>&
	{
		return vReprioritizedHoldData;
	}
	auto GetCopyOfReprioritizedHoldData() const -> std::vector<HoldReplayResult>
	{
		return vReprioritizedHoldData;
	}
	void SetReprioritizedHoldData(const std::vector<HoldReplayResult>& v)
	{
		vReprioritizedHoldData = v;
	}
	auto GetReprioritizedMineData() const
	  -> const std::vector<MineReplayResult>&
	{
		return vReprioritizedMineData;
	}
	auto GetCopyOfReprioritizedMineData() const -> std::vector<MineReplayResult>
	{
		return vReprioritizedMineData;
	}
	void SetReprioritizedMineData(const std::vector<MineReplayResult>& v)
	{
		vReprioritizedMineData = v;
	}

	auto GetRelevantMissData() const -> const std::vector<MissReplayResult>&
	{
		if (useReprioritizedNoterows) {
			return vReprioritizedMissData;
		} else {
			return vMissReplayDataVector;
		}
	}
	auto GetRelevantHoldData() const -> const std::vector<HoldReplayResult>& {
		if (useReprioritizedNoterows) {
			return vReprioritizedHoldData;
		} else {
			return vHoldReplayDataVector;
		}
	}
	auto GetRelevantMineData() const -> const std::vector<MineReplayResult>& {
		if (useReprioritizedNoterows) {
			return vReprioritizedMineData;
		} else {
			return vMineReplayDataVector;
		}
	}


	ReplayType GetReplayType() const
	{
		if (!InputData.empty()) {
			// detailed data
			return ReplayType_Input;
		} else if (!vTrackVector.empty()) {
			// column data
			return ReplayType_V2;
		} else if (!vNoteRowVector.empty()) {
			// no column/extra data
			return ReplayType_V1;
		} else {
			// it probably isn't loaded
			return ReplayType_Invalid;
		}
	}

	/// true for V2 and InputData
	auto HasColumnData() const -> bool
	{
		const auto t = GetReplayType();
		return t >= ReplayType_V2 && t < NUM_ReplayType;
	}

	auto WriteReplayData() -> bool;
	auto WriteInputData() -> bool;
	auto LoadReplayData() -> bool;
	auto HasReplayData() -> bool;
	auto HasWrittenReplayData() -> bool;

	/// Corrects missing fields for InputData.
	/// Will only work for InputData backed by loaded NoteData
	auto FillInBlanksForInputData() -> bool;

	/// Generate ReplayV2 Data from InputData.
	/// The main use of this is for rescoring the classic way
	auto GeneratePrimitiveVectors() -> bool;
	/// Generate Noterow vector using online timestamp replay format
	auto GenerateNoterowsFromTimestamps() -> bool;
	/// Generate InputData using any ReplayData
	auto GenerateInputData() -> bool;

	/// Used for recalculating notedata nearest noterows.
	/// Uses a different algorithm than "closest note" to rejudge the data.
	auto ReprioritizeInputData() -> bool;

	/// Generate events used for playing back replay in gameplay
	auto GeneratePlaybackEvents(int startRow = 0)
	  -> std::map<int, std::vector<PlaybackEvent>>;

	/// For Stats and ReplaySnapshots
	auto GenerateJudgeInfoAndReplaySnapshots(int startingRow = 0,
											 float timingScale = 1.F) -> bool;

	// Instead of making some complex iterator...
	// Just offer both solutions
	/// Returns map of columns to a set of rows which are dropped
	/// See which columns have drops using this
	auto GenerateDroppedHoldColumnsToRowsMap(int startRow = 0)
	  -> std::map<int, std::set<int>>;
	/// Returns a map of rows to a set of columns which are dropped
	/// See which rows have drops using this
	auto GenerateDroppedHoldRowsToColumnsMap(int startRow = 0)
	  -> std::map<int, std::set<int>>;

	/// Offsets can be really weird - Remove all impossible offsets
	void ValidateOffsets();

	/// Noterows for Replay/Input Data can just ... shift...
	/// This is only a bit uncommon. It can happen if you simply shift
	/// an entire chart forward or backwards in noterows only.
	/// So this function tries to correct data by shifting it
	/// to match the existing notedata.
	/// If this function returns false, reloading InputData is recommended.
	auto ValidateInputDataNoterows() -> bool;

	/// Used to validate that converting input data to replay data
	/// produces correct and equal output vs replay data alone.
	/// This is not meant to ever be used outside of debug.
	void VerifyInputDataAndReplayData();

	/// Used to validate that converting replay data to input data
	/// produces correct and equal output vs replay data alone.
	/// This is not meant to ever be used outside of debug.
	void VerifyGeneratedInputDataMatchesReplayData();

	auto GetHighScore() -> HighScore*;
	auto GetSteps() -> Steps*;
	auto GetNoteData(Steps* pSteps = nullptr, bool bTransform = true)
	  -> NoteData;
	auto GetTimingData() -> TimingData*;
	auto GetStyle() -> const Style*;

	auto GetReplaySnapshotForNoterow(int row)
	  -> std::shared_ptr<ReplaySnapshot>;

	/// A check to see if the Replay has an RNG seed, if it uses shuffle.
	auto CanSafelyTransformNoteData() -> bool;

	void Unload()
	{
		useReprioritizedNoterows = false;
		generatedInputData = false;

		// stats
		m_ReplaySnapshotMap.clear();

		ClearReprioritizedVectors();

		// replay data
		ClearPrimitiveVectors();

		InputData.clear();
		vMissReplayDataVector.clear();
		vHoldReplayDataVector.clear();
		vMineReplayDataVector.clear();

		InputData.shrink_to_fit();
		vMissReplayDataVector.shrink_to_fit();
		vHoldReplayDataVector.shrink_to_fit();
		vMineReplayDataVector.shrink_to_fit();

		// extra online data "backups"
		vOnlineOffsetVector.clear();
		vOnlineNoteRowVector.clear();
		vOnlineTrackVector.clear();
		vOnlineTapNoteTypeVector.clear();
		vOnlineOffsetVector.shrink_to_fit();
		vOnlineNoteRowVector.shrink_to_fit();
		vOnlineTrackVector.shrink_to_fit();
		vOnlineTapNoteTypeVector.shrink_to_fit();
	}

	/// Lua
	void PushSelf(lua_State* L);

  private:
	auto LoadReplayDataBasic(const std::string& replayDir = BASIC_REPLAY_DIR)
	  -> bool;
	auto LoadReplayDataFull(const std::string& replayDir = FULL_REPLAY_DIR)
	  -> bool;
	auto LoadInputData(const std::string& replayDir = INPUT_DATA_DIR) -> bool;

	auto LoadStoredOnlineData() -> bool;

	/// For V1 or earlier replays lacking column data, we need to assume
	/// information. Make it all up. This fills in the column data using
	/// NoteData. This also provides TapNoteTypes
	auto GenerateReplayV2DataPresumptively() -> bool;

	/// Setting the mod string is handled separately.
	/// Use this to set mods, as long as a scorekey is given.
	auto SetHighScoreMods() -> void;

	void ClearPrimitiveVectors() {
		vOffsetVector.clear();
		vNoteRowVector.clear();
		vTrackVector.clear();
		vTapNoteTypeVector.clear();
		vOnlineReplayTimestampVector.clear();

		vOffsetVector.shrink_to_fit();
		vNoteRowVector.shrink_to_fit();
		vTrackVector.shrink_to_fit();
		vTapNoteTypeVector.shrink_to_fit();
		vOnlineReplayTimestampVector.shrink_to_fit();
	}

	void ClearReprioritizedVectors() {
		vReprioritizedMissData.clear();
		vReprioritizedHoldData.clear();
		vReprioritizedMineData.clear();
		vReprioritizedMissData.shrink_to_fit();
		vReprioritizedHoldData.shrink_to_fit();
		vReprioritizedMineData.shrink_to_fit();
	}

	bool isOnlineScore() const {
		return scoreKey.find("Online_") != std::string::npos;
	}

	std::map<int, ReplaySnapshot> m_ReplaySnapshotMap{};
	JudgeInfo judgeInfo{};

	// optimization for ReplaySnapshot generation
	// filled out by RegenerateJudgmentInfo
	std::set<int> significantNoterows{};

	// for snapshot stuff and rescoring
	// set by Lua, data filled by ReprioritizeInputData
	bool useReprioritizedNoterows = false;
	std::vector<MissReplayResult> vReprioritizedMissData{};
	std::vector<MineReplayResult> vReprioritizedMineData{};
	std::vector<HoldReplayResult> vReprioritizedHoldData{};

	std::string scoreKey{};
	std::string chartKey{};
	float fMusicRate = 1.F;
	float fSongOffset = 0.F;
	float fGlobalOffset = 0.F;
	std::string mods{};
	int rngSeed = 0;

	std::vector<InputDataEvent> InputData{};
	std::vector<MissReplayResult> vMissReplayDataVector{};
	std::vector<float> vOffsetVector{};
	std::vector<int> vNoteRowVector{};
	std::vector<int> vTrackVector{};
	std::vector<TapNoteType> vTapNoteTypeVector{};
	std::vector<HoldReplayResult> vHoldReplayDataVector{};
	std::vector<MineReplayResult> vMineReplayDataVector{};
	std::vector<float> vOnlineReplayTimestampVector{};

	// mainly useful for the noterow reprioritization stuff
	// because if we switch that and it is generated, force generate
	// the reason is that given only v2, we generate inputdata
	// that input data outputs a separate set of v2 data
	// and reloading that v2 data generates wrong input data
	// so this just refreshes the whole process
	bool generatedInputData = false;

	/////
	// storage of vectors temporarily for online scores only
	std::vector<float> vOnlineOffsetVector{};
	std::vector<int> vOnlineNoteRowVector{};
	std::vector<int> vOnlineTrackVector{};
	std::vector<TapNoteType> vOnlineTapNoteTypeVector{};
	/////
};

#endif
