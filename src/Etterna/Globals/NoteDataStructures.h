// Only NoteData structures independent of the rest of Stepmania live here

#ifndef __NDSTRUCTS__
#define __NDSTRUCTS__

#include <string>
#include <vector>

struct NoteInfo
{
	unsigned int notes;
	float rowTime;
};

struct ChartInfo
{
	std::string difficultyName;
	std::vector<NoteInfo> notes;
};

struct DifficultyRating
{
	float overall;
	float stream;
	float jumpstream;
	float handstream;
	float stamina;
	float jack;
	float chordjack;
	float technical;
};

struct ChartRating
{
	std::string difficultyName;
	DifficultyRating rating;
};

#endif
