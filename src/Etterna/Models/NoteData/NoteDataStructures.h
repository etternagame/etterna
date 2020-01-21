// Only NoteData structures independent of the rest of Stepmania live here

#ifndef __NDSTRUCTS__
#define __NDSTRUCTS__

struct NoteInfo
{
	unsigned int notes;
	float rowTime;
};

struct NoteInfo2
{
	int notes;
	int rowTime;
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

#endif
