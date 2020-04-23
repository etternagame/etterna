#pragma once
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include <vector>

// For internal, must be preprocessor defined
#if defined(MINADLL_COMPILE) && defined(_WIN32)
#define MINACALC_API __declspec(dllexport)
#endif

// For Stepmania
#ifndef MINACALC_API
#define MINACALC_API
#endif

  typedef std::vector<float>
	SDiffs;
typedef std::vector<SDiffs> MinaSD;

typedef std::vector<std::vector<float>> Finger;
typedef std::vector<Finger> ProcessedFingers;
typedef std::vector<float> JackSeq;

enum CalcPatternMod
{
	OHJump,
	Anchor,
	Roll,
	HS,
	Jump,
	MSD, // is pulling stam adjusted values atm
	PtLoss,
	ModCount,
	None
};

/*	The difficulties of each hand tend to be independent from one another. This
is not absolute, as in the case of polyrhythm trilling. However the goal of the
calculator is to estimate the difficulty of a file given the physical properties
of such, and not to evalute the difficulty of reading (which is much less
quantifiable). It is both less accurate and logically incorrect to attempt to
assert a single difficulty for both hands for a given interval of time in a
file, so most of the internal calculator operations are done after splitting up
each track of the chart into their respective phalangeal parents. */
class Hand
{
  public:
	/*	Spits out a rough estimate of difficulty based on the ms values within
	the interval The std::vector passed to it is the std::vector of ms values within each
	interval, and not the full std::vector of intervals. */
	float CalcMSEstimate(std::vector<float>& v);

	// Wraps the three prepatory functions below
	void InitHand(Finger& f1, Finger& f2, float ts);

	/*	Averages nps and ms estimates for difficulty to get a rough initial
	value. This is relatively robust as patterns that get overrated by nps
	estimates are underrated by ms estimates, and vice versa. Pattern modifiers
	are used to adjust for circumstances in which this is not true. The result
	is output to v_itvdiff. */
	void InitDiff(Finger& f1, Finger& f2);

	// Totals up the points available for each interval
	void InitPoints(Finger& f1, Finger& f2);

	// Self explanatory
	void SetTimingScale(float ts) { timingscale = ts; }

	/*	The stamina model works by asserting a minimum difficulty relative to
	the supplied player skill level for which the player's stamina begins to
	wane. Experience in both gameplay and algorithm testing has shown the
	appropriate value to be around 0.8. The multiplier is scaled to the
	proportionate difference in player skill. */
	std::vector<float> StamAdjust(float x, std::vector<float> diff);

	/*	For a given player skill level x, invokes the function used by wife
	scoring to assert the average of the distribution of point gain for each
	interval and then tallies up the result to produce an average total number
	of points achieved by this hand. */
	float CalcInternal(float x, bool stam, bool nps, bool js, bool hs);

	std::vector<float> ohjumpscale;
	std::vector<float> rollscale;
	std::vector<float> hsscale;
	std::vector<float> jumpscale;
	std::vector<float> anchorscale;
	std::vector<int> v_itvpoints;	// Point allotment for each interval
	std::vector<float> v_itvNPSdiff; // Calculated difficulty for each interval
	std::vector<float> v_itvMSdiff;  // Calculated difficulty for each interval
	std::vector<float> debug;		 // debug info placement
	std::vector<float> finalMSDvals; // cancer cancer cancer cancer
	std::vector<float> pointslost;
  private:
	const bool SmoothDifficulty =
	  true; // Do we moving average the difficulty intervals?

	float timingscale; // Timingscale for use in the point proportion function
	float jumpstreamscaler = 0.975f;
	float handstreamscaler = 0.92f;
	float finalscaler = 2.564f * 1.05f * 1.1f * 1.10f * 1.10f *
						1.025f; // multiplier to standardize baselines

	// Stamina Model params
	const float ceil = 1.08f;	// stamina multiplier max
	const float mag = 355.f;	 // multiplier generation scaler
	const float fscale = 2000.f; // how fast the floor rises (it's lava)
	const float prop =
	  0.75f; // proportion of player difficulty at which stamina tax begins
};

class Calc
{
  public:
	/*	Primary calculator function that wraps everything else. Initializes the
	hand objects and then runs the chisel function under varying circumstances
	to estimate difficulty for each different skillset. Currently only
	overall/stamina are being produced. */
	std::vector<float> CalcMain(const std::vector<NoteInfo>& NoteInfo, float timingscale, float score_goal);

	// redo these asap
	std::vector<float> JackStamAdjust(std::vector<float>& j, float x, bool jackstam);
	float JackLoss(std::vector<float>& j, float x, bool jackstam);
	JackSeq SequenceJack(const std::vector<NoteInfo>& NoteInfo, int t);

	CalcPatternMod debugMod = CalcPatternMod::None;

	int numitv;
	int fastwalk(const std::vector<NoteInfo>& NoteInfo);

	/*	Splits up the chart by each hand and calls ProcessFinger on each "track"
	before passing
	the results to the hand initialization functions. Also passes the input
	timingscale value. */
	void InitializeHands(const std::vector<NoteInfo>& NoteInfo, float ts);

	/*	Slices the track into predefined intervals of time. All taps within each
	interval have their ms values from the last note in the same column
	calculated and the result is spit out
	into a new Finger object, or std::vector of std::vectors of floats (ms from last note
	in the track). */
	Finger ProcessFinger(const std::vector<NoteInfo>& NoteInfo, int t);

	// How many buttons do you press for this chart (currently hardcoded,
	// clearly)
	int numTracks = 4;

	float vb = 0.f;
	float vt = 0.f;

	// Derivative calc params
	float MusicRate = 1.f;
	float MaxPoints = 0.f; // Total points achievable in the file
	float Scoregoal =
	  0.93f; // What proportion of the total points are we trying to get
	const float toffset = 0.f;
	void TotalMaxPoints(); // Counts up the total points and assigns it

	/*	Recursive non-linear calculation function. A player skill is asserted
	and the calcultor calls the calcinternal functions for each hand and adds up
	the calculated average points attained per attempt at the given skill level
	for the given chart. This function will iterate until the percentage
	obtained is greater than or equal to the scoregoal variable. The output
	accuracy resolution can be set by either reducing the initial increment or
	by increasing the starting iteration. */
	float Chisel(float pskill,
				 float res,
				 int iter,
				 bool stam,
				 bool jack,
				 bool nps,
				 bool js,
				 bool hs);

	std::vector<float> OHJumpDownscaler(const std::vector<NoteInfo>& NoteInfo,
								   int t1,
								   int t2);
	std::vector<float> Anchorscaler(const std::vector<NoteInfo>& NoteInfo,
							   int t1,
							   int t2);
	std::vector<float> HSDownscaler(const std::vector<NoteInfo>& NoteInfo);
	std::vector<float> JumpDownscaler(const std::vector<NoteInfo>& NoteInfo);
	std::vector<float> RollDownscaler(Finger f1, Finger f2);
	void Purge();
	float techscaler = 0.97f;

	Hand left_hand;
	Hand right_hand;
  private:
	std::vector<std::vector<int>> nervIntervals;

	// Const calc params
	const bool SmoothPatterns =
	  true; // Do we moving average the pattern modifier intervals?
	const float IntervalSpan = 0.5f; // Intervals of time we slice the chart at
	const bool logpatterns = false;

	float dumbvalue = 1.f;
	int dumbcounter = 0;

	JackSeq j0;
	JackSeq j1;
	JackSeq j2;
	JackSeq j3;
};

MINACALC_API std::vector<float>
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo,
		   float musicrate,
		   float goal);
MINACALC_API MinaSD
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo);

MINACALC_API void
MinaSDCalcDebug(const std::vector<NoteInfo>& NoteInfo,
				float musicrate,
				float goal,
				std::vector<std::vector<float>>& handInfo,
				CalcPatternMod cpm = CalcPatternMod::None);
MINACALC_API int
GetCalcVersion();

/*
To get debug output:
1. Turn off the built in showlog window
2. Do StepMania.exe >> out.txt in cmd
*/
