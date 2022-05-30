#ifndef REPLAY_H
#define REPLAY_H

#include "Etterna/Models/Misc/EnumHelper.h"
#include "ReplayConstantsAndTypes.h"

struct HighScore;

class Replay
{
  public:
	Replay();
	Replay(HighScore* hs);
	~Replay();

	auto GetOffsetVector() const -> const std::vector<float>& {
		return vOffsetVector;
	}
	auto GetCopyOfOffsetVector() const -> std::vector<float> {
		return vOffsetVector;
	}
	void SetOffsetVector(const std::vector<float>& v) { vOffsetVector = v; }

	auto GetNoteRowVector() const -> const std::vector<int>& {
		return vNoteRowVector;
	}
	auto GetCopyOfNoteRowVector() const -> std::vector<int> {
		return vNoteRowVector;
	}
	void SetNoteRowVector(const std::vector<int>& v) { vNoteRowVector = v; }

	auto GetTrackVector() const -> const std::vector<int>& {
		return vTrackVector;
	}
	auto GetCopyOfTrackVector() const -> std::vector<int> {
		return vTrackVector;
	}
	void SetTrackVector(const std::vector<int>& v) { vTrackVector = v; }

	auto GetTapNoteTypeVector() const -> const std::vector<TapNoteType>& {
		return vTapNoteTypeVector;
	}
	auto GetCopyOfTapNoteTypeVector() const -> std::vector<TapNoteType> {
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

	auto GetOnlineReplayTimestampVector() const -> const std::vector<float>& {
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

	auto GetInputDataVector() const -> const std::vector<InputDataEvent>& {
		return InputData;
	}
	auto GetCopyOfInputDataVector() const -> std::vector<InputDataEvent> {
		return InputData;
	}
	void SetInputDataVector(const std::vector<InputDataEvent>& v)
	{
		InputData = v;
	}

	auto GetScoreKey() const -> std::string {
		return scoreKey;
	}
	void SetScoreKey(std::string& key) {
		scoreKey = key;
	}
	auto GetChartKey() const -> std::string {
		return chartKey;
	}
	void SetChartKey(std::string& key) {
		chartKey = key;
	}
	auto GetMusicRate() const -> float {
		return fMusicRate;
	}
	void SetMusicRate(float f) {
		fMusicRate = f;
	}
	auto GetSongOffset() const -> float {
		return fSongOffset;
	}
	void SetSongOffset(float f) {
		fSongOffset = f;
	}
	auto GetGlobalOffset() const -> float {
		return fGlobalOffset;
	}
	void SetGlobalOffset(float f) {
		fGlobalOffset = f;
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

	auto WriteReplayData() -> bool;
	auto WriteInputData() -> bool;
	auto LoadInputData() -> bool;
	auto LoadReplayData() -> bool;
	auto HasReplayData() -> bool;

	void Unload() {
		vNoteRowVector.clear();
		vOffsetVector.clear();
		vTrackVector.clear();
		vTapNoteTypeVector.clear();
		vHoldReplayDataVector.clear();
		vMineReplayDataVector.clear();
		vOnlineReplayTimestampVector.clear();
		InputData.clear();

		vNoteRowVector.shrink_to_fit();
		vOffsetVector.shrink_to_fit();
		vTrackVector.shrink_to_fit();
		vTapNoteTypeVector.shrink_to_fit();
		vHoldReplayDataVector.shrink_to_fit();
		vMineReplayDataVector.shrink_to_fit();
		vOnlineReplayTimestampVector.shrink_to_fit();
		InputData.shrink_to_fit();
	}

	// Lua
	void PushSelf(lua_State* L);

  private:
	auto LoadReplayDataBasic(const std::string& dir) -> bool;
	auto LoadReplayDataFull(const std::string& dir) -> bool;

	std::string scoreKey{};
	std::string chartKey{};
	float fMusicRate = 1.F;
	float fSongOffset = 0.F;
	float fGlobalOffset = 0.F;

	std::vector<InputDataEvent> InputData;
	std::vector<float> vOffsetVector;
	std::vector<int> vNoteRowVector;
	std::vector<int> vTrackVector;
	std::vector<TapNoteType> vTapNoteTypeVector;
	std::vector<HoldReplayResult> vHoldReplayDataVector;
	std::vector<MineReplayResult> vMineReplayDataVector;
	std::vector<float> vOnlineReplayTimestampVector;

};

#endif
