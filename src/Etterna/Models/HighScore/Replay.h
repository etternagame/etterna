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
	Replay(const HighScore* hs);
	Replay(std::string chartKey,
		   float musicRate,
		   float songOffset,
		   float globalOffset,
		   int rngSeed);
	~Replay();

	inline const std::string GetBasicPath() const
	{
		return BASIC_REPLAY_DIR + scoreKey;
	}

	inline const std::string GetFullPath() const
	{
		return FULL_REPLAY_DIR + scoreKey;
	}

	inline const std::string GetInputPath() const
	{
		return INPUT_DATA_DIR + scoreKey;
	}

	inline const std::string GetOnlinePath() const
	{
		return ONLINE_DATA_DIR + scoreKey;
	}

	const std::vector<float>& GetOffsetVector() const
	{
		return vOffsetVector;
	}
	std::vector<float> GetCopyOfOffsetVector() const
	{
		return vOffsetVector;
	}
	void SetOffsetVector(const std::vector<float>& v) { vOffsetVector = v; }

	const std::vector<int>& GetNoteRowVector() const
	{
		return vNoteRowVector;
	}
	std::vector<int> GetCopyOfNoteRowVector() const
	{
		return vNoteRowVector;
	}
	void SetNoteRowVector(const std::vector<int>& v) { vNoteRowVector = v; }

	const std::vector<int>& GetTrackVector() const
	{
		return vTrackVector;
	}
	std::vector<int> GetCopyOfTrackVector() const
	{
		return vTrackVector;
	}
	void SetTrackVector(const std::vector<int>& v) { vTrackVector = v; }

	const std::vector<TapNoteType>& GetTapNoteTypeVector() const
	{
		return vTapNoteTypeVector;
	}
	std::vector<TapNoteType> GetCopyOfTapNoteTypeVector() const
	{
		return vTapNoteTypeVector;
	}
	void SetTapNoteTypeVector(const std::vector<TapNoteType>& v)
	{
		vTapNoteTypeVector = v;
	}

	const std::vector<HoldReplayResult>& GetHoldReplayDataVector() const
	{
		return vHoldReplayDataVector;
	}
	std::vector<HoldReplayResult> GetCopyOfHoldReplayDataVector() const
	{
		return vHoldReplayDataVector;
	}
	void SetHoldReplayDataVector(const std::vector<HoldReplayResult>& v)
	{
		vHoldReplayDataVector = v;
	}

	const std::vector<MineReplayResult>& GetMineReplayDataVector() const
	{
		return vMineReplayDataVector;
	}
	std::vector<MineReplayResult> GetCopyOfMineReplayDataVector() const
	{
		return vMineReplayDataVector;
	}
	void SetMineReplayDataVector(const std::vector<MineReplayResult>& v)
	{
		vMineReplayDataVector = v;
	}

	const std::vector<float>& GetOnlineReplayTimestampVector() const
	{
		return vOnlineReplayTimestampVector;
	}
	std::vector<float> GetCopyOfOnlineReplayTimestampVector() const
	{
		return vOnlineReplayTimestampVector;
	}
	void SetOnlineReplayTimestampVector(const std::vector<float>& v)
	{
		vOnlineReplayTimestampVector = v;
	}

	const std::vector<InputDataEvent>& GetInputDataVector() const
	{
		return InputData;
	}
	std::vector<InputDataEvent> GetCopyOfInputDataVector() const
	{
		return InputData;
	}
	void SetInputDataVector(const std::vector<InputDataEvent>& v)
	{
		InputData = v;
	}

	const std::vector<MissReplayResult>& GetMissReplayDataVector() const
	{
		return vMissReplayDataVector;
	}
	std::vector<MissReplayResult> GetCopyOfMissReplayDataVector() const
	{
		return vMissReplayDataVector;
	}
	void SetMissReplayDataVector(const std::vector<MissReplayResult>& v)
	{
		vMissReplayDataVector = v;
	}

	const std::map<int, ReplaySnapshot>& GetReplaySnapshotMap() const
	{
		return m_ReplaySnapshotMap;
	}
	std::map<int, ReplaySnapshot> GetCopyOfReplaySnapshotMap() const
	{
		return m_ReplaySnapshotMap;
	}
	void SetReplaySnapshotMap(const std::map<int, ReplaySnapshot>& m)
	{
		m_ReplaySnapshotMap = m;
	}

	JudgeInfo& GetJudgeInfo() { return judgeInfo; }
	JudgeInfo GetCopyOfJudgeInfo() const { return judgeInfo; }
	void SetJudgeInfo(const JudgeInfo& ji) { judgeInfo = ji; }

	std::string GetScoreKey() const { return scoreKey; }
	void SetScoreKey(std::string& key) { scoreKey = key; }
	std::string GetChartKey() const { return chartKey; }
	void SetChartKey(std::string& key) { chartKey = key; }
	float GetMusicRate() const { return fMusicRate; }
	void SetMusicRate(float f) { fMusicRate = f; }
	float GetSongOffset() const { return fSongOffset; }
	void SetSongOffset(float f) { fSongOffset = f; }
	float GetGlobalOffset() const { return fGlobalOffset; }
	void SetGlobalOffset(float f) { fGlobalOffset = f; }
	int GetRngSeed() const { return rngSeed; }
	void SetRngSeed(int seed) { rngSeed = seed; }
	std::string GetModifiers() const { return mods; }
	void SetModifiers(std::string& modstr) { mods = modstr; }

	void SetUseReprioritizedNoteRows(bool b)
	{
		if (b != useReprioritizedNoterows) {
			if (IsOnlineScore()) {
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
	bool UsingReprioritizedNoteRows()
	{
		return useReprioritizedNoterows;
	}
	const std::vector<MissReplayResult>& GetReprioritizedMissData() const
	{
		return vReprioritizedMissData;
	}
	std::vector<MissReplayResult> GetCopyOfReprioritizedMissData() const
	{
		return vReprioritizedMissData;
	}
	void SetReprioritizedMissData(const std::vector<MissReplayResult>& v) {
		vReprioritizedMissData = v;
	}
	const std::vector<HoldReplayResult>& GetReprioritizedHoldData() const
	{
		return vReprioritizedHoldData;
	}
	std::vector<HoldReplayResult> GetCopyOfReprioritizedHoldData() const
	{
		return vReprioritizedHoldData;
	}
	void SetReprioritizedHoldData(const std::vector<HoldReplayResult>& v)
	{
		vReprioritizedHoldData = v;
	}
	const std::vector<MineReplayResult>& GetReprioritizedMineData() const
	{
		return vReprioritizedMineData;
	}
	std::vector<MineReplayResult> GetCopyOfReprioritizedMineData() const
	{
		return vReprioritizedMineData;
	}
	void SetReprioritizedMineData(const std::vector<MineReplayResult>& v)
	{
		vReprioritizedMineData = v;
	}

	const std::vector<MissReplayResult>& GetRelevantMissData() const
	{
		if (useReprioritizedNoterows) {
			return vReprioritizedMissData;
		} else {
			return vMissReplayDataVector;
		}
	}
	const std::vector<HoldReplayResult>& GetRelevantHoldData() const
	{
		if (useReprioritizedNoterows) {
			return vReprioritizedHoldData;
		} else {
			return vHoldReplayDataVector;
		}
	}
	const std::vector<MineReplayResult>& GetRelevantMineData() const
	{
		if (useReprioritizedNoterows) {
			return vReprioritizedMineData;
		} else {
			return vMineReplayDataVector;
		}
	}

	void IngestInputData(bool ispress,
						 int col,
						 int row,
						 float musicsecs,
						 float offset,
						 TapNoteType tnt,
						 TapNoteSubType tnst)
	{
		InputData.emplace_back(
		  ispress, col, musicsecs, row, -offset, tnt, tnst);
	}

	void IngestHoldDrop(int col, int row, TapNoteSubType tnst)
	{
		HoldReplayResult hrr;
		hrr.row = row;
		hrr.track = col;
		hrr.subType = tnst;
		vHoldReplayDataVector.push_back(hrr);
	}

	void IngestMineHit(int col, int row)
	{
		MineReplayResult mrr;
		mrr.row = row;
		mrr.track = col;
		vMineReplayDataVector.push_back(mrr);
	}

	void IngestMissData(int col, int row, TapNoteType tnt, TapNoteSubType tnst)
	{
		MissReplayResult mrr;
		mrr.row = row;
		mrr.track = col;
		mrr.tapNoteType = tnt;
		mrr.tapNoteSubType = tnst;
		vMissReplayDataVector.push_back(mrr);
	}

	void IngestV2Data(int col, int row, float offset, TapNoteType tnt)
	{
		vTrackVector.push_back(col);
		vNoteRowVector.push_back(row);
		vOffsetVector.push_back(offset);
		vTapNoteTypeVector.push_back(tnt);
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
	bool HasColumnData() const
	{
		const auto t = GetReplayType();
		return t >= ReplayType_V2 && t < NUM_ReplayType;
	}

	bool WriteReplayData();
	bool WriteInputData();
	bool LoadReplayData();
	bool HasReplayData() const;
	bool HasWrittenReplayData() const;

	/// Corrects missing fields for InputData.
	/// Will only work for InputData backed by loaded NoteData
	bool FillInBlanksForInputData();

	/// Generate ReplayV2 Data from InputData.
	/// The main use of this is for rescoring the classic way
	bool GeneratePrimitiveVectors();
	/// Generate Noterow vector using online timestamp replay format
	bool GenerateNoterowsFromTimestamps();
	/// Generate InputData using any ReplayData
	bool GenerateInputData();

	/// Used for recalculating notedata nearest noterows.
	/// Uses a different algorithm than "closest note" to rejudge the data.
	bool ReprioritizeInputData();

	/// Generate events used for playing back replay in gameplay
	std::map<int, std::vector<PlaybackEvent>> GeneratePlaybackEvents(
	  int startRow = 0);

	/// Generate an event for replay playback for only one InputData element
	std::map<int, std::vector<PlaybackEvent>>
	GeneratePlaybackEventForInputDataHead();

	/// For Stats and ReplaySnapshots
	bool GenerateJudgeInfoAndReplaySnapshots(int startingRow = 0,
											 float timingScale = 1.F);

	// Instead of making some complex iterator...
	// Just offer both solutions
	/// Returns map of columns to a set of rows which are dropped
	/// See which columns have drops using this
	std::map<int, std::set<int>> GenerateDroppedHoldColumnsToRowsMap(
	  int startRow = 0);
	/// Returns a map of rows to a set of columns which are dropped
	/// See which rows have drops using this
	std::map<int, std::set<int>> GenerateDroppedHoldRowsToColumnsMap(
	  int startRow = 0);

	/// Generate the event required for spectator playback
	std::map<int, std::set<int>> GenerateDroppedHoldColumnsToRowsMapFromHead();

	/// Offsets can be really weird - Remove all impossible offsets
	void ValidateOffsets();

	/// Noterows for Replay/Input Data can just ... shift...
	/// This is only a bit uncommon. It can happen if you simply shift
	/// an entire chart forward or backwards in noterows only.
	/// So this function tries to correct data by shifting it
	/// to match the existing notedata.
	/// If this function returns false, reloading InputData is recommended.
	bool ValidateInputDataNoterows();

	/// Used to validate that converting input data to replay data
	/// produces correct and equal output vs replay data alone.
	/// This is not meant to ever be used outside of debug.
	void VerifyInputDataAndReplayData();

	/// Used to validate that converting replay data to input data
	/// produces correct and equal output vs replay data alone.
	/// This is not meant to ever be used outside of debug.
	void VerifyGeneratedInputDataMatchesReplayData();

	HighScore* GetHighScore() const;
	Steps* GetSteps() const;
	NoteData GetNoteData(Steps* pSteps = nullptr, bool bTransform = true);
	TimingData* GetTimingData() const;
	const Style* GetStyle() const;

	std::shared_ptr<ReplaySnapshot> GetReplaySnapshotForNoterow(int row);

	/// A check to see if the Replay has an RNG seed, if it uses shuffle.
	bool CanSafelyTransformNoteData();

	bool IsOnlineScore() const
	{
		return scoreKey.find("Online_") != std::string::npos;
	}

	bool IsSpectateScore() const
	{
		return scoreKey.find("SPECTATE") != std::string::npos;
	}

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

	/// Setting the mod string is handled separately.
	/// Use this to set mods, as long as a scorekey is given.
	void SetHighScoreMods();

	/// Lua
	void PushSelf(lua_State* L);

  private:
	bool LoadReplayDataBasic(const std::string& replayDir = BASIC_REPLAY_DIR);
	bool LoadReplayDataFull(const std::string& replayDir = FULL_REPLAY_DIR);
	bool LoadInputData(const std::string& replayDir = INPUT_DATA_DIR);
	bool LoadOnlineDataFromDisk(const std::string& replayDir = ONLINE_DATA_DIR);
	bool LoadStoredOnlineData();

	/// For V1 or earlier replays lacking column data, we need to assume
	/// information. Make it all up. This fills in the column data using
	/// NoteData. This also provides TapNoteTypes
	bool GenerateReplayV2DataPresumptively();

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

	// if we failed to load data, dont try again and waste time
	bool attemptedToLoadInputData = false;
	bool loadResultInputData = false;
	bool attemptedToLoadReplayV2 = false;
	bool loadResultReplayV2 = false;
	bool attemptedToLoadReplayV1 = false;
	bool loadResultReplayV1 = false;

	bool LoadedInputData(bool b) {
		attemptedToLoadInputData = true;
		loadResultInputData = b;
		return b;
	}
	bool LoadedReplayV2(bool b) {
		attemptedToLoadReplayV2 = true;
		loadResultReplayV2 = b;
		return b;
	}
	bool LoadedReplayV1(bool b) {
		attemptedToLoadReplayV1 = true;
		loadResultReplayV1 = b;
		return b;
	}

	/////
	// storage of vectors temporarily for online scores only
	std::vector<float> vOnlineOffsetVector{};
	std::vector<int> vOnlineNoteRowVector{};
	std::vector<int> vOnlineTrackVector{};
	std::vector<TapNoteType> vOnlineTapNoteTypeVector{};
	/////
};

#endif
