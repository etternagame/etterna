#ifndef REPLAY_H
#define REPLAY_H

#include "Etterna/Models/Misc/EnumHelper.h"
#include "ReplayConstantsAndTypes.h"

class Replay
{
	Replay();

  public:
	[[nodiscard]] auto GetOffsetVector() const -> const std::vector<float>&;
	[[nodiscard]] auto GetCopyOfOffsetVector() const -> std::vector<float>;

	[[nodiscard]] auto GetNoteRowVector() const -> const std::vector<int>&;
	[[nodiscard]] auto GetCopyOfNoteRowVector() const -> std::vector<int>;

	[[nodiscard]] auto GetTrackVector() const -> const std::vector<int>&;
	[[nodiscard]] auto GetCopyOfTrackVector() const -> std::vector<int>;

	[[nodiscard]] auto GetTapNoteTypeVector() const
	  -> const std::vector<TapNoteType>&;
	[[nodiscard]] auto GetCopyOfTapNoteTypeVector() const
	  -> std::vector<TapNoteType>;

	[[nodiscard]] auto GetHoldReplayDataVector() const
	  -> const std::vector<HoldReplayResult>&;
	[[nodiscard]] auto GetCopyOfHoldReplayDataVector() const
	  -> std::vector<HoldReplayResult>;

	[[nodiscard]] auto GetMineReplayDataVector() const
	  -> const std::vector<MineReplayResult>&;
	[[nodiscard]] auto GetCopyOfMineReplayDataVector() const
	  -> std::vector<MineReplayResult>;

	[[nodiscard]] auto GetCopyOfSetOnlineReplayTimestampVector() const
	  -> std::vector<float>;

	[[nodiscard]] auto GetInputDataVector() const
	  -> const std::vector<InputDataEvent>&;
	[[nodiscard]] auto GetCopyOfInputDataVector() const
	  -> std::vector<InputDataEvent>;

	[[nodiscard]] auto GetReplayType() const -> int;

	auto WriteReplayData() -> bool;
	auto WriteInputData() -> bool;
	auto LoadInputData() -> bool;
	auto LoadReplayData() -> bool;
	auto LoadReplayDataBasic(const std::string& dir) -> bool;
	auto LoadReplayDataFull(const std::string& dir) -> bool;
	virtual auto HasReplayData() -> bool;
	void UnloadReplayData();

	void PushSelf(lua_State* L);

  private:
	std::string scoreKey{};
	std::string chartKey{};

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
