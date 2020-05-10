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

// number of pattern mods yes i know this is dumb help
// we want to group stamina adjustment with the others for
// display purposes but we don't want it when initializing pattern arrays
// because it doesn't get generated until after we're done
const static int ModCount = NUM_CalcPatternMod;

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
	the interval The vector passed to it is the vector of ms values within each
	interval, and not the full vector of intervals. */
	static float CalcMSEstimate(std::vector<float>& input);

	/*	Averages nps and ms estimates for difficulty to get a rough initial
	value. This is relatively robust as patterns that get overrated by nps
	estimates are underrated by ms estimates, and vice versa. Pattern modifiers
	are used to adjust for circumstances in which this is not true. The result
	is output to v_itvNPSdiff and v_itvMSdiff. */
	void InitBaseDiff(Finger& f1, Finger& f2);

	// I don't know why this was ever being done in the internal loop, only the
	// stam adjusted difficulties were dependent on a player_skill input, these
	// values are static. Just calculate them for each skillset after pattern
	// mods are done. For reasons we want to calculate stam mod on a different
	// vector than what we apply the stam mod to, so calculate those as well.
	// Yes this makes sense. 
	void InitAdjDiff();

	// Totals up the points available for each interval
	void InitPoints(const Finger& f1, const Finger& f2);

	/*	The stamina model works by asserting a minimum difficulty relative to
	the supplied player skill level for which the player's stamina begins to
	wane. Experience in both gameplay and algorithm testing has shown the
	appropriate value to be around 0.8. The multiplier is scaled to the
	proportionate difference in player skill. */
	// just recycle the stam_adj_diff vector directly in this function
	// sometimes 
	void StamAdjust(float x, int ss, bool debug = false);

	/*	For a given player skill level x, invokes the function used by wife
	scoring to assert the average of the distribution of point gain for each
	interval and then tallies up the result to produce an average total number
	of points achieved by this hand. */
	// return value is true if a player can no longer reach scoregoal even if
	// they get 100% of the remaining points
	void CalcInternal(float& gotpoints,
					  float& x,
					  int ss,
					  bool stam,
					  bool debug = false);

	std::vector<float> doot[ModCount];
	std::vector<int> v_itvpoints; // Point allotment for each interval
	std::vector<float> soap[NUM_CalcDiffValue]; // Calculated difficulty for each interval

	// not necessarily self extraplanetary
	// apply stam model to these (but output is sent to stam_adj_diff, not modified here)
	std::vector<float> base_adj_diff[NUM_Skillset];
	// but use these as the input for model
	std::vector<float> base_diff_for_stam_mod[NUM_Skillset];

	// pattern adjusted difficulty, allocate only once, stam needs to be based
	// on the above, and it needs to be recalculated every time the player_skill
	// value changes, again based on the above, technically we could use the
	// skill_stamina element of the arrays to store this and save an allocation
	// but that might just be too confusing idk
	std::vector<float> stam_adj_diff;
	std::vector<std::vector<std::vector<float>>> debugValues;

  private:
	const bool SmoothDifficulty =
	  true; // Do we moving average the difficulty intervals?
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
	bool capssr = true;	// set to true for scores, false for cache
	int numitv;

	/*	Splits up the chart by each hand and calls ProcessFinger on each "track"
	before passing the results to the hand initialization functions. Also passes
	the input timingscale value. Hardcode a limit for nps and if we hit it just
	return max value for the calc and move on, there's no point in calculating
	values for 500 nps joke files. Mem optimization can be better if we only
	allow 100 maximum notes for a single half second interval, return value is
	whether or not we should continue calculation */
	bool InitializeHands(const std::vector<NoteInfo>& NoteInfo,
						 float music_rate,
						 float offset);

	/*	Slices the track into predefined intervals of time. All taps within each
	interval have their ms values from the last note in the same column
	calculated and the result is spit out
	into a new Finger object, or vector of vectors of floats (ms from last note
	in the track). */
	Finger ProcessFinger(const std::vector<NoteInfo>& NoteInfo,
						 unsigned int t,
						 float music_rate,
						 float offset,
						 bool& joke_file_mon);

	// Derivative calc params
	int MaxPoints = 0; // Total points achievable in the file
	void TotalMaxPoints(); // Counts up the total points and assigns it

	/*	Returns estimate of player skill needed to achieve score goal on chart.
	 *  The player_skill parameter gives an initial guess and floor for player
	 * skill. Resolution relates to how precise the answer is. Additional
	 * parameters give specific skill sets being tested for.*/
	float Chisel(float player_skill,
				 float resolution,
				 float score_goal,
				 int ss,	// skillset
				 bool stamina,
				 bool debugoutput = false);

	void TheThingLookerFinderThing(const std::vector<NoteInfo>& NoteInfo,
								   float music_rate,
								   std::vector<float> doot[]);

	// nerf psuedo chords that are flams into oblivion
	void SetFlamJamMod(const std::vector<NoteInfo>& NoteInfo,
					   std::vector<float> doot[],
					   float& music_rate);

	void SetStreamMod(const std::vector<NoteInfo>& NoteInfo,
					 std::vector<float> doot[ModCount], float music_rate);

	void SetAnchorMod(const std::vector<NoteInfo>& NoteInfo,
									unsigned int t1,
									unsigned int t2,
									std::vector<float> doot[ModCount]);

	// no longer going to necessarily be downscalers - that they were
	// was a structural flaw of the old calc
	void SetHSMod(const std::vector<NoteInfo>& NoteInfo,
									std::vector<float> doot[ModCount]);
	void SetJumpMod(const std::vector<NoteInfo>& NoteInfo,
									  std::vector<float> doot[ModCount]);
	void SetCJMod(const std::vector<NoteInfo>& NoteInfo,
					std::vector<float> doot[ModCount]);

	// run pattern mods that require specific sequencing at the same time to
	// avoid iterating through all rows of the noteinfo more than once
	// ok well we do it once per hand and we can probably solve that but...
	void SetSequentialDownscalers(
	  const std::vector<NoteInfo>& NoteInfo,
	  unsigned int t1,
	  unsigned int t2,
	  float music_rate,
	  std::vector<float> doot[ModCount]);

	void WideRangeRollScaler(const std::vector<NoteInfo>& NoteInfo,
							 unsigned int t1,
							 unsigned int t2,
							 float music_rate,
							 std::vector<float> doot[ModCount]);
	void WideRangeJumptrillScaler(const std::vector<NoteInfo>& NoteInfo,
								  unsigned int t1,
								  unsigned int t2,
								  float music_rate,
								  std::vector<float> doot[ModCount]);
	void WideRangeAnchorScaler(const std::vector<NoteInfo>& NoteInfo,
							   float music_rate,
							   std::vector<float> doot[ModCount]);
	void WideRangeBalanceScaler(const std::vector<NoteInfo>& NoteInfo,
								float music_rate,
								std::vector<float> doot[ModCount]);
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
				std::vector<std::vector<std::vector<std::vector<float>>>>& handInfo);
MINACALC_API int
GetCalcVersion();
