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

typedef std::vector<std::vector<float>> MinaSD;
typedef std::vector<std::vector<float>> Finger;
typedef std::vector<Finger> ProcessedFingers;
typedef std::vector<float> JackSeq;

/*	The difficulties of each hand tend to be independent from one another. This
is not absolute, as in the case of polyrhythm trilling. However the goal of the
calculator is to estimate the difficulty of a file given the physical properties
of such, and not to evalute the difficulty of reading (which is much less
quantifiable). It is both less accurate and logically incorrect to attempt to
assert a single difficulty for both hands for a given interval of time in a
file, so most of the internal calculator operations are done after splitting up
each track of the chart into their respective phalangeal parents. */  // This is stupid but whatever
class Hand
{
  public:
	/*	Spits out a rough estimate of difficulty based on the ms values within
	the interval The vector passed to it is the vector of ms values within each
	interval, and not the full vector of intervals. */
	static float CalcMSEstimate(std::vector<float>& input);

	/*	Averages nps and ms estimates for difficulty to get a rough initial
	value. This is relatively robust as patterns that get overrated by nps
	estimates are underrated by ms estimates, and vice versa. Pattern modifiers
	are used to adjust for circumstances in which this is not true. The result
	is output to v_itvNPSdiff and v_itvMSdiff. */
	void InitDiff(Finger& f1, Finger& f2);

	// Totals up the points available for each interval
	void InitPoints(const Finger& f1, const Finger& f2);

	/*	The stamina model works by asserting a minimum difficulty relative to
	the supplied player skill level for which the player's stamina begins to
	wane. Experience in both gameplay and algorithm testing has shown the
	appropriate value to be around 0.8. The multiplier is scaled to the
	proportionate difference in player skill. */
	std::vector<float> StamAdjust(float x, std::vector<float>& diff);

	/*	For a given player skill level x, invokes the function used by wife
	scoring to assert the average of the distribution of point gain for each
	interval and then tallies up the result to produce an average total number
	of points achieved by this hand. */
	float CalcInternal(float x, bool stam, bool nps, bool js, bool hs, bool debug);

	std::vector<float> ohjumpscale, rollscale, hsscale, jumpscale, anchorscale;
	std::vector<int> v_itvpoints; // Point allotment for each interval
	std::vector<float> v_itvNPSdiff,
	  v_itvMSdiff, pureMSdiff; // Calculated difficulty for each interval
	std::vector<std::vector<float>> debugValues;

  private:
	const bool SmoothDifficulty =
	  true; // Do we moving average the difficulty intervals?

	float finalscaler = 2.564f * 1.05f * 1.1f * 1.10f * 1.10f *
						1.025f; // multiplier to standardize baselines

	// Stamina Model params
	const float ceil = 1.09f;	// stamina multiplier max
	const float mag = 355.f;	 // multiplier generation scaler
	const float fscale = 2500.f; // how fast the floor rises (it's lava)
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
	std::vector<float> CalcMain(const std::vector<NoteInfo>& NoteInfo,
								float music_rate,
								float score_goal);

	// redo these asap
	static float JackLoss(const std::vector<float>& j, float x);
	static JackSeq SequenceJack(const std::vector<NoteInfo>& NoteInfo,
								unsigned int t,
								float music_rate);

	bool debugmode = false;
	int numitv;

	/*	Splits up the chart by each hand and calls ProcessFinger on each "track"
	before passing
	the results to the hand initialization functions. Also passes the input
	timingscale value. */
	void InitializeHands(const std::vector<NoteInfo>& NoteInfo,
						 float music_rate);

	/*	Slices the track into predefined intervals of time. All taps within each
	interval have their ms values from the last note in the same column
	calculated and the result is spit out
	into a new Finger object, or vector of vectors of floats (ms from last note
	in the track). */
	Finger ProcessFinger(const std::vector<NoteInfo>& NoteInfo,
						 unsigned int t,
						 float music_rate);

	// Derivative calc params
	float MaxPoints = 0.f; // Total points achievable in the file
	void TotalMaxPoints(); // Counts up the total points and assigns it

	/*	Returns estimate of player skill needed to achieve score goal on chart.
	 *  The player_skill parameter gives an initial guess and floor for player
	 * skill. Resolution relates to how precise the answer is. Additional
	 * parameters give specific skill sets being tested for.*/
	float Chisel(float player_skill,
				 float resolution,
				 float score_goal,
				 bool stamina,
				 bool jack,
				 bool nps,
				 bool js,
				 bool hs,
				 bool debugoutput = false);

	std::vector<float> OHJumpDownscaler(const std::vector<NoteInfo>& NoteInfo,
										unsigned int t1,
										unsigned int t2);
	std::vector<float> Anchorscaler(const std::vector<NoteInfo>& NoteInfo,
									unsigned int t1,
									unsigned int t2);
	std::vector<float> HSDownscaler(const std::vector<NoteInfo>& NoteInfo);
	std::vector<float> JumpDownscaler(const std::vector<NoteInfo>& NoteInfo);
	std::vector<float> RollDownscaler(const std::vector<NoteInfo>& NoteInfo,
									  unsigned int t1,
									  unsigned int t2,
									  float music_rate);

	Hand left_hand;
	Hand right_hand;

  private:
	std::vector<std::vector<int>> nervIntervals;

	// Const calc params
	const bool SmoothPatterns =
	  true; // Do we moving average the pattern modifier intervals?
	const float IntervalSpan = 0.5f; // Intervals of time we slice the chart at
	const bool logpatterns = false;
	float fingerbias = 1.f;

	JackSeq j0;
	JackSeq j1;
	JackSeq j2;
	JackSeq j3;
};

MINACALC_API std::vector<float>
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo, float musicrate, float goal);
MINACALC_API MinaSD
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo);
MINACALC_API void
MinaSDCalcDebug(const std::vector<NoteInfo>& NoteInfo,
				float musicrate,
				float goal,
				std::vector<std::vector<std::vector<float>>>& handInfo);
MINACALC_API int
GetCalcVersion();
