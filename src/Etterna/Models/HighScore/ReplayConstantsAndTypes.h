#ifndef REPLAY_CONSTS_H
#define REPLAY_CONSTS_H

#include "Etterna/Models/Misc/NoteTypes.h"

// contains only tap offset data for rescoring/plots
const std::string BASIC_REPLAY_DIR = "Save/Replays/";

// contains freeze drops and mine hits as well as tap
// offsets; fully "rewatchable"
const std::string FULL_REPLAY_DIR = "Save/ReplaysV2/";

// contains input data files corresponding to replays
const std::string INPUT_DATA_DIR = "Save/InputData/";

const std::string NO_MODS = "none";

/// enum values defined by Replay.GetReplayType()
enum ReplayType
{
	ReplayType_V0,	  // ????
	ReplayType_V1,	  // contains only note info
	ReplayType_V2,	  // contains column info and type info with note info
	ReplayType_Input, // contains full input data
	NUM_ReplayType,
	ReplayType_Invalid,
};

auto
ReplayTypeToString(ReplayType replayType) -> const std::string&;
auto
StringToReplayType(const std::string& str) -> ReplayType;
LuaDeclareType(ReplayType);

struct InputDataEvent
{
	bool is_press;
	int column;
	// input data saves song seconds here
	// instead of beats
	// the reason is that we want it to be able to be parsed by a human
	// and analyzed externally
	// beats cant easily be analyzed without song bpm info
	float songPositionSeconds;
	int nearestTapNoterow;
	float offsetFromNearest;
	TapNoteType nearestTapNoteType = TapNoteType_Invalid;
	// really only applies for holds and rolls
	TapNoteSubType nearestTapNoteSubType = TapNoteSubType_Invalid;

	// for inputdata remappings only
	int reprioritizedNearestNoterow = -1;
	float reprioritizedOffsetFromNearest = 1.F;
	TapNoteType reprioritizedNearestTapNoteType = TapNoteType_Invalid;
	TapNoteSubType reprioritizedNearestTapNoteSubType = TapNoteSubType_Invalid;

	InputDataEvent()
	{
		is_press = false;
		column = -1;
		songPositionSeconds = 0.F;
		nearestTapNoterow = 0;
		offsetFromNearest = 0.F;
	}

	InputDataEvent(bool press,
				   int col,
				   float songPos,
				   int row,
				   float offset,
				   TapNoteType tapnotetype,
				   TapNoteSubType tapnotesubtype)
	  : is_press(press)
	  , column(col)
	  , songPositionSeconds(songPos)
	  , nearestTapNoterow(row)
	  , offsetFromNearest(offset)
	  , nearestTapNoteType(tapnotetype)
	  , nearestTapNoteSubType(tapnotesubtype)
	{
	}

	InputDataEvent(const InputDataEvent& other) {
		is_press = other.is_press;
		column = other.column;
		songPositionSeconds = other.songPositionSeconds;
		nearestTapNoterow = other.nearestTapNoterow;
		offsetFromNearest = other.offsetFromNearest;
		nearestTapNoteType = other.nearestTapNoteType;
		nearestTapNoteSubType = other.nearestTapNoteSubType;
		reprioritizedNearestNoterow = other.reprioritizedNearestNoterow;
		reprioritizedOffsetFromNearest = other.reprioritizedOffsetFromNearest;
		reprioritizedNearestTapNoteType = other.reprioritizedNearestTapNoteType;
		reprioritizedNearestTapNoteSubType =
		  other.reprioritizedNearestTapNoteSubType;
	}

	/// Lua
	void PushSelf(lua_State* L);
};

struct MineReplayResult
{
	int row;
	int track; // column

	MineReplayResult()
	{
		row = 0;
		track = 0;
	}
};

struct MissReplayResult
{
	int row;
	int track; // column
	TapNoteType tapNoteType = TapNoteType_Invalid;
	TapNoteSubType tapNoteSubType = TapNoteSubType_Invalid;

	MissReplayResult()
	{
		row = 0;
		track = 0;
	}

	MissReplayResult(int row,
					 int col,
					 TapNoteType tapnotetype,
					 TapNoteSubType tapnotesubtype)
	  : row(row)
	  , track(col)
	  , tapNoteType(tapnotetype)
	  , tapNoteSubType(tapnotesubtype)
	{
	}
};

struct HoldReplayResult
{
	int row;
	int track; // column
	TapNoteSubType subType;

	HoldReplayResult()
	{
		row = 0;
		track = 0;
		subType = TapNoteSubType_Invalid;
	}
};

struct TapReplayResult
{
	int row;
	int track;			   // column
	float offset;		   // 0
	TapNoteType type;	   // typically mines, holds, rolls, etc

	TapReplayResult()
	{
		row = 0;
		track = 0;
		offset = 0.F;
		type = TapNoteType_Invalid;
	}

	TapReplayResult(const TapReplayResult& other) {
		row = other.row;
		track = other.track;
		offset = other.offset;
		type = other.type;
	}
};

struct PlaybackEvent
{
	int noterow;
	float songPositionSeconds;
	int track;		// column
	bool isPress;	// tap or release

	// only applies if the event judges a note
	// to prevent events triggering wrong judgments
	int noterowJudged = -1;
	float offset = 0.F;

	PlaybackEvent()
	{
		noterow = 0;
		songPositionSeconds = 0.F;
		track = 0;
		isPress = true;
	}

	PlaybackEvent(int noterow, float songPositionSeconds, int track, bool isPress)
	  : noterow(noterow)
	  , songPositionSeconds(songPositionSeconds)
	  , track(track)
	  , isPress(isPress)
	{
	}

	bool isJudgeEvent() const { return noterowJudged != -1; }
};

// Basically contains a record for any given noterow of the essential info about
// the Player But only the info we can simply derive from the given ReplayData
struct ReplaySnapshot
{
	// Contains Marv->Miss and Mines Hit
	int judgments[NUM_TapNoteScore] = { 0 };
	// Hold note scores
	int hns[NUM_HoldNoteScore] = { 0 };
	float curwifescore = 0.F;
	float maxwifescore = 0.F;
	float standardDeviation = 0.F;
	float mean = 0.F;

	/// Lua
	void PushSelf(lua_State* L);
};

struct JudgeInfo
{
	std::map<int, std::vector<TapReplayResult>> trrMap{};
	std::map<int, std::vector<HoldReplayResult>> hrrMap{};

	// horrible inefficiency :think:
	std::map<float, std::vector<TapReplayResult>> trrMapByElapsedTime{};
	std::map<float, std::vector<HoldReplayResult>> hrrMapByElapsedTime{};
};

#endif
