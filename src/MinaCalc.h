#pragma once
#define PRINTF(a,b)
#include "global.h"
#include "GameState.h"
#include "RageUtil.h"
#include "Notedata.h"
#include "Steps.h"
#include "Song.h"
//#include <vector>
//#include <map>
//#include <set>
//#include <iterator>

// For internal, must be preprocessor defined
#ifdef MINADLL_COMPILE
#define MINACALC_API __declspec(dllexport)
#endif

// For Stepmania
#ifndef MINADLL_COMPILE
#define MINACALC_API
#endif

typedef vector<float> SDiffs;
typedef vector<SDiffs> MinaSD;

typedef vector<vector<float>> Finger;
typedef vector<Finger> ProcessedFingers;
typedef vector<float> JackSeq;

/*	The difficulties of each hand tend to be independent from one another. This is not
absolute, as in the case of polyrhythm trilling. However the goal of the calculator is
to estimate the difficulty of a file given the physical properties of such, and not to
evalute the difficulty of reading (which is much less quantifiable). It is both less
accurate and logically incorrect to attempt to assert a single difficulty for both hands
for a given interval of time in a file, so most of the internal calculator operations
are done after splitting up each track of the chart into their respective phalangeal
parents. */
class Hand {
public:
	/*	Spits out a rough estimate of difficulty based on the ms values within the interval
	The vector passed to it is the vector of ms values within each interval, and not the
	full vector of intervals. */
	float CalcMSEstimate(vector<float>& v);

	// Wraps the three prepatory functions below
	void InitHand(Finger & f1, Finger & f2, float ts);

	/*	Averages nps and ms estimates for difficulty to get a rough initial value. This is
	relatively robust as patterns that get overrated by nps estimates are underrated by
	ms estimates, and vice versa. Pattern modifiers are used to adjust for circumstances
	in which this is not true. The result is output to v_itvdiff. */
	void InitDiff(Finger & f1, Finger & f2);

	// Totals up the points available for each interval
	void InitPoints(Finger & f1, Finger & f2);

	// Self explanatory
	void SetTimingScale(float ts) { timingscale = ts; }

	/*	The stamina model works by asserting a minimum difficulty relative to the supplied
	player skill level for which the player's stamina begins to wane. Experience in both
	gameplay and algorithm testing has shown the appropriate value to be around 0.8. The
	multiplier is scaled to the proportionate difference in player skill. */
	vector<float> StamAdjust(float x);

	/*	For a given player skill level x, invokes the function used by wife scoring to
	assert the average of the distribution of point gain for each interval and then
	tallies up the result to produce an average total number of points achieved by this
	hand. */
	float CalcInternal(float x, bool stam);

	vector<float> ohjumpscale;
	vector<float> rollscale;

	vector<int> v_itvpoints;	// Point allotment for each interval

private:
	const bool SmoothDifficulty = false;	// Do we moving average the difficulty intervals?
	vector<float> v_itvdiff;	// Calculated difficulty for each interval
	float timingscale;			// Timingscale for use in the point proportion function
	float finalscaler = 2.564f;	// multiplier to standardize baselines

								// Stamina Model params
	const float ceil = 1.15f;	// stamina multiplier max
	const float mag = 225.f;	// multiplier generation scaler
	const float fscale = 2000;	// how fast the floor rises (it's lava)
	const float prop = 0.8f;	// proportion of player difficulty at which stamina tax begins
};

class Calc {
public:
	/*	Primary calculator function that wraps everything else. Initializes the hand objects and
	then runs the chisel function under varying circumstances to estimate difficulty for each
	different skillset. Currently only overall/stamina are being produced. */
	vector<float> CalcMain(NoteData & nd, vector<float>& etaner, float timingscale);

	// redo these asap
	vector<float> JackStamAdjust(vector<float>& j, float x);
	float JackLoss(vector<float>& j, float x);
	JackSeq SequenceJack(NoteData & nd, vector<float>& etaner, int t);

	int numitv;
	int fastwalk(NoteData & nd, vector<float>& etaner);

	/*	Splits up the chart by each hand and calls ProcessFinger on each "track" before passing
	the results to the hand initialization functions. Also passes the input timingscale value. */
	void InitializeHands(NoteData & nd, vector<float>& etaner, float ts);

	/*	Slices the track into predefined intervals of time. All taps within each interval have
	their ms values from the last note in the same column calculated and the result is spit out
	into a new Finger object, or vector of vectors of floats (ms from last note in the track). */
	Finger ProcessFinger(NoteData & nd, vector<float>& etaner, int t);

	// How many buttons do you press for this chart (currently hardcoded, clearly)
	int numTracks = 4;

	// Derivative calc params
	float MusicRate = 1.f;
	float MaxPoints = 0.f;		// Total points achievable in the file
	float Scoregoal = 0.93f;	// What proportion of the total points are we trying to get
	void TotalMaxPoints();		// Counts up the total points and assigns it

								/*	Recursive non-linear calculation function. A player skill is asserted and the calcultor
								calls the calcinternal functions for each hand and adds up the calculated average points
								attained per attempt at the given skill level for the given chart. This function will iterate
								until the percentage obtained is greater than or equal to the scoregoal variable. The output
								accuracy resolution can be set by either reducing the initial increment or by increasing the
								starting iteration. */
	float Chisel(float pskill, float res, int iter, bool stam, bool jack);

	vector<float> OHJumpDownscaler(NoteData & nd, int t1, int t2);
	vector<float> RollDownscaler(Finger f1, Finger f2);
private:
	vector<vector<int>> nervIntervals;

	// Const calc params
	const bool SmoothPatterns = false;	// Do we moving average the pattern modifier intervals?

	const float IntervalSpan = 0.5f;	// Intervals of time we slice the chart at

	Hand* left = new Hand;
	Hand* right = new Hand;

	JackSeq j0;
	JackSeq j1;
	JackSeq j2;
	JackSeq j3;
};

MINACALC_API vector<float> MinaSDCalc(NoteData& nd, vector<float>& etaner, float musicrate, float goal, float timingscale, bool negbpms);
MINACALC_API MinaSD MinaSDCalc(NoteData& nd, vector<float>& etaner, float goal, float timingscale, bool negbpms);