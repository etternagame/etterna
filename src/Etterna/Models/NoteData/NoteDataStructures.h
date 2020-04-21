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


enum CalcDebugValue
{
	OHJump,  // pattern mod (% values between 0-1)
	Anchor,  // pattern mod (% values between 0-1)
	Roll,	// pattern mod (% values between 0-1)
	HS,		 // pattern mod (% values between 0-1)
	Jump,	// pattern mod (% values between 0-1)
	BaseNPS, // unadjusted base nps difficulty
	BaseMS,  // unadjusted base ms difficulty
	BaseMSD, // unadjusted weighted values
	MSD,	 // pattern and stam adjusted difficulty values
	PtLoss,  // expected points lost
	StamMod, // stam adjustment value (% values between 1-ciel)
	DebugCount,
	None
};

#endif
