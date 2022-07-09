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

	InputDataEvent()
	{
		is_press = false;
		column = -1;
		songPositionSeconds = 0.F;
		nearestTapNoterow = 0;
		offsetFromNearest = 0.F;
	}

	InputDataEvent(bool press, int col, float songPos, int row, float offset)
	  : is_press(press)
	  , column(col)
	  , songPositionSeconds(songPos)
	  , nearestTapNoterow(row)
	  , offsetFromNearest(offset)
	{
	}
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
	int offsetAdjustedRow; // row assigned later on for full replays

	TapReplayResult()
	{
		row = 0;
		track = 0;
		offset = 0.F;
		type = TapNoteType_Invalid;
		offsetAdjustedRow = 0;
	}
};

struct PlaybackEvent
{
	float beat;
	int track;	// column
	bool isTap;	// tap or release

	PlaybackEvent()
	{
		beat = 0.F;
		track = 0;
		isTap = true;
	}

	PlaybackEvent(float beat, int track, bool isTap)
	  : beat(beat)
	  , track(track)
	  , isTap(isTap)
	{
	}
};

#endif
