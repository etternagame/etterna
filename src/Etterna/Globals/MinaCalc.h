#pragma once
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include <vector>
#include <array>

// For internal, must be preprocessor defined
#if defined(MINADLL_COMPILE) && defined(_WIN32)
#define MINACALC_API __declspec(dllexport)
#endif

// For Stepmania
#ifndef MINACALC_API
#define MINACALC_API
#endif

typedef std::vector<std::vector<float>> MinaSD;
using Finger = std::vector<std::vector<float>>;
using ProcessedFingers = std::vector<Finger>;

class Calc;

// should be able to handle 1hr 54min easily
static const int max_intervals = 40000;

enum hands
{
	left_hand,
	right_hand,
	num_hands,
};

class Hand
{
  public:
	// I don't know why this was ever being done in the internal loop, only the
	// stam adjusted difficulties were dependent on a player_skill input, these
	// values are static. Just calculate them for each skillset after pattern
	// mods are done. For reasons we want to calculate stam mod on a different
	// vector than what we apply the stam mod to, so calculate those as well.
	// Yes this makes sense.
	void InitAdjDiff(Calc& calc);

	// Totals up the points available for each interval
	void InitPoints(const Finger& f1, const Finger& f2);

	/*	The stamina model works by asserting a minimum difficulty relative to
	the supplied player skill level for which the player's stamina begins to
	wane. Experience in both gameplay and algorithm testing has shown the
	appropriate value to be around 0.8. The multiplier is scaled to the
	proportionate difference in player skill. */
	// just recycle the stam_adj_diff vector directly in this function
	// sometimes
	void StamAdjust(float x, int ss, Calc& calc, bool debug = false);

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
					  Calc& calc,
					  bool debug = false);

	// Point allotment for each interval
	std::vector<int> v_itvpoints;

	std::vector<std::vector<std::vector<float>>> debugValues;

	int hi = 0;
};

class Calc
{
  public:
	/*	Primary calculator function that wraps everything else. Initializes the
	hand objects and then runs the chisel function under varying circumstances
	to estimate difficulty for each different skillset. Currently only
	overall/stamina are being produced. */
	auto CalcMain(const std::vector<NoteInfo>& NoteInfo,
				  float music_rate,
				  float score_goal) -> std::vector<float>;

	bool debugmode = false;
	bool ssr = true; // set to true for scores, false for cache

	/*	Splits up the chart by each hand and calls ProcessFinger on each "track"
	before passing the results to the hand initialization functions. Also passes
	the input timingscale value. Hardcode a limit for nps and if we hit it just
	return max value for the calc and move on, there's no point in calculating
	values for 500 nps joke files. Mem optimization can be better if we only
	allow 100 maximum notes for a single half second interval, return value is
	whether or not we should continue calculation */
	auto InitializeHands(const std::vector<NoteInfo>& NoteInfo,
						 float music_rate,
						 float offset) -> bool;

	/*	Slices the track into predefined intervals of time. All taps within each
	interval have their ms values from the last note in the same column
	calculated and the result is spit out
	into a new Finger object, or vector of vectors of floats (ms from last note
	in the track). */
	auto ProcessFinger(const std::vector<NoteInfo>& NoteInfo,
					   unsigned int t,
					   float music_rate,
					   float offset,
					   bool& joke_file_mon) -> Finger;

	// Derivative calc params
	int MaxPoints = 0;	   // Total points achievable in the file
	void TotalMaxPoints(); // Counts up the total points and assigns it

	/*	Returns estimate of player skill needed to achieve score goal on chart.
	 *  The player_skill parameter gives an initial guess and floor for player
	 * skill. Resolution relates to how precise the answer is. Additional
	 * parameters give specific skill sets being tested for.*/
	auto Chisel(float player_skill,
				float resolution,
				float score_goal,
				int ss, // skillset
				bool stamina,
				bool debugoutput = false) -> float;

	Hand l_hand;
	Hand r_hand;

  private:
	std::vector<std::vector<int>> nervIntervals;

	const float IntervalSpan = 0.5F; // Intervals of time we slice the chart at

  public:
	// holds pattern mods
	std::array<std::array<std::array<float, max_intervals>, NUM_CalcPatternMod>,
			   num_hands>
	  doot;

	// Calculated difficulty for each interval
	std::array<std::array<std::array<float, max_intervals>, NUM_CalcDiffValue>,
			   num_hands>
	  soap;

	// not necessarily self extraplanetary
	// apply stam model to these (but output is sent to stam_adj_diff, not
	// modified here)
	std::array<std::array<std::array<float, max_intervals>, NUM_Skillset>,
			   num_hands>
	  base_adj_diff;
	// but use these as the input for model
	std::array<std::array<std::array<float, max_intervals>, NUM_Skillset>,
			   num_hands>
	  base_diff_for_stam_mod;

	// pattern adjusted difficulty, allocate only once, stam needs to be based
	// on the above, and it needs to be recalculated every time the player_skill
	// value changes, again based on the above, technically we could use the
	// skill_stamina element of the arrays to store this and save an allocation
	// but that might just be too confusing idk
	std::array<float, max_intervals> stam_adj_diff;

	/* NOTE: all _incoming_ diffs should be stored as MS values, and only
	 * converted to scaled NPS on the way out */
};

MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo,
		   float musicrate,
		   float goal,
		   Calc* calc = nullptr) -> std::vector<float>;
MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo, Calc* calc = nullptr)
  -> MinaSD;
MINACALC_API void
MinaSDCalcDebug(
  const std::vector<NoteInfo>& NoteInfo,
  float musicrate,
  float goal,
  std::vector<std::vector<std::vector<std::vector<float>>>>& handInfo);
MINACALC_API auto
GetCalcVersion() -> int;
