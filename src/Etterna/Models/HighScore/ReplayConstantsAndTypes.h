#ifndef REPLAY_CONSTS_H
#define REPLAY_CONSTS_H

#include "Etterna/Models/Misc/NoteTypes.h"

/// enum values defined by Replay.GetReplayType()
enum ReplayType
{
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
	{
		is_press = press;
		column = col;
		songPositionSeconds = songPos;
		nearestTapNoterow = row;
		offsetFromNearest = offset;
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

#endif
