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

enum Skillset
{
	Skill_Overall,
	Skill_Stream,
	Skill_Jumpstream,
	Skill_Handstream,
	Skill_Stamina,
	Skill_JackSpeed,
	Skill_Chordjack,
	Skill_Technical,
	NUM_Skillset,
	Skillset_Invalid,
};

// we do actually want to register these with lua i guess
enum CalcPatternMod
{
	StreamMod,
	JS,   // pattern mod (values between 0-1)
	JSS,		// pattern mod (values between 0-1)
	JSJ,		// pattern mod (values between 0-1)
	HS,  // pattern mod (values between 0-1)
	HSS, // pattern mod (values between 0-1)
	HSJ, // pattern mod (values between 0-1)
	CJ,		// pattern mod (values between 0-1)
	CJS,		// pattern mod (values between 0-1)
	CJJ,		// pattern mod (values between 0-1)
	CJQuad,
	OHJumpMod, // pattern mod (values between 0-1)
	OHJPropComp,
	OHJSeqComp,
	OHJMaxSeq,
	OHJCCTaps,
	OHJHTaps,
	CJOHJump,
	CJOHJPropComp,
	CJOHJSeqComp,
	Anchor, // pattern mod (values between 0.9 - ~ 1.1)
	Roll,   // pattern mod (values between 0-1)
	OHTrill,
	Chaos,
	FlamJam,
	WideRangeRoll,
	WideRangeJumptrill,
	WideRangeBalance,
	WideRangeAnchor,
	TheThing,
	RanMan,
	RanLen,
	RanAnchLen,
	RanAnchLenMod,
	RanJack,
	RanOHT,
	RanOffS,
	RanPropAll,
	RanPropOff,
	RanPropOHT,
	RanPropOffS,
	RanPropJack,
	NUM_CalcPatternMod,
	CalcPatternMod_Invalid,
};
enum CalcDiffValue
{
	BaseNPS, // unadjusted base nps difficulty
	BaseMS,  // unadjusted base ms difficulty
	BaseMSD, // unadjusted weighted values
	MSD,	 // pattern and stam adjusted difficulty values
	NUM_CalcDiffValue,
	CalcDiffValue_Invalid,
};
enum CalcDebugMisc
{
	PtLoss, // expected points loss (not really a diff thing but w.e)
	JackPtLoss,
	StamMod,// stam adjust (values between 1- ~1.15)
	JackStamMod,
	NUM_CalcDebugMisc,
	CalcDebugMisc_Invalid,
};

#endif
