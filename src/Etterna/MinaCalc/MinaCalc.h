#pragma once
#include "../Models/NoteData/NoteDataStructures.h"
#include <string>
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

/// Defines a structure containing skillset values for every rate to cache MSDs.
using MinaSD = std::vector<std::vector<float>>;

class Calc;

/** This defines the base size for each interval-based vector in MinaCalc.
* Each interval is one half second. If any situation arises in which the
* number of intervals required is above 1000, the vectors will resize to fit.
* 1000 intervals should be enough to fit most files (8.3 minutes)
*/
static constexpr int default_interval_count = 1000;

/** This is a hard cap to file length in terms of intervals to prevent massive
* memory allocations. Almost every file that exists (except a handful) fit this.
*/
static constexpr int max_intervals = 100000;

/** This is a hard cap on how many rows should fit into an interval.
* Intervals are one half second. If this is reached, this implies that the
* file has a 100 nps burst. Nobody could ever possibly hit that (in 4k).
* Any file that reaches this in any interval is skipped.
*/
static constexpr int max_rows_for_single_interval = 50;

/** Defining which hands exist (the left and right).
* Values in this enum are referred to when iterating hands and when checking
* to see how many hands exist.
*/
enum hands
{
	left_hand,
	right_hand,
	num_hands,
};

/// Both hands, left to right
static constexpr hands both_hands[num_hands] = { left_hand, right_hand };

/** Each NoteInfo is translated into this struct and stored in Calc::adj_ni.
* The point is to precalculate a bit of the information for clarity and speed.
* The drawback is primarily memory allocation related.
*/
struct RowInfo
{
	/** Binary representation of notes in the row.
	* Look at the notes in a .sm for example. Mirror it, and it is this.
	* Tap on leftmost column: 0001 ... Tap on rightmost column: 1000
	*/
	unsigned row_notes = 0U;

	/// 1-4 referring to if the row is a tap, jump, hand, or quad.
	int row_count = 0;

	/// Counting the left handed and right handed notes in this row.
	std::array<int, num_hands> hand_counts = { 0, 0 };

	/// Rate-scaled time of this row.
	float row_time = 0.F;
};

/// Main driver class for the difficulty calculator as a whole.
class Calc
{
  public:
	/** Primary calculator function that wraps everything else. Runs notedata
	* through ulbu which builds the base diff values and then runs the chisel
	* function over the produced diff vectors for each skillset. The main
	* drawback of this approach is files with multiple skillsets tend to get
	* significantly underrated
	*/
	auto CalcMain(const std::vector<NoteInfo>& NoteInfo,
				  float music_rate,
				  float score_goal) -> std::vector<float>;

	/// For debug output. Should only ever be true at music select. 
	bool debugmode = false;

	/// Set true for score related output, and false for MSD caching.
	bool ssr = true;

	/// Set true to force calc params to load outside debug mode.
	bool loadparams = false;

  private:
	/** Splits up the chart by each hand and processes them individually to
	* produce hand specific base difficulty values, which are then passed to
	* the chisel functions. Hardcode a limit for nps (100) and if we hit it just
	* return max value for the calc and move on, there's no point in
	* calculating values for 500 nps joke files.
	*
	* Return value is whether or not we should skip calculation.
	*/
	auto InitializeHands(const std::vector<NoteInfo>& NoteInfo,
						 float music_rate,
						 float offset) -> bool;

	/** Returns estimate of player skill needed to achieve score goal on chart.
	* The player_skill parameter gives an initial guess and floor for player
	* skill. Resolution relates to how precise the answer is. Additional
	* parameters give specific skill sets being tested for.
	*/
	auto Chisel(float player_skill,
				float resolution,
				float score_goal,
				Skillset ss,
				bool stamina,
				bool debugoutput = false) -> float;

	/** Once per hand, adjust the difficulty vectors using patternmod values.
	* Difficulty vectors: jack_diff, base_adj_diff, base_diff_for_stam_mod.
	* Iterates over each interval, and every skillset for each interval.
	* Skips iterations of Overall and Stamina (unaffected by patternmods).
	*/
	static inline void InitAdjDiff(Calc& calc, const int& hand);

  public:
	/** Each Calc instance created sets up the interval related vectors.
	* Their default size is default_interval_count.
	*/
	Calc() { resize_interval_dependent_vectors(default_interval_count); }
	
	/** For each interval, there are up to max_rows_for_single_interval entries
	* of RowInfo. This is precalculated by fast_walk_and_check_for_skip.
	* Calc iteration uses RowInfo from this instead of iterating NoteInfo.
	*/
	std::vector<std::array<RowInfo, max_rows_for_single_interval>>
	  adj_ni;

	/// Number of rows in each interval.
	std::vector<int> itv_size{};

	/** Number of points per interval per hand.
	* This is equivalent to the number of notes * 2.
	* Set up by nps::actual_cancer
	*/
	std::array<std::vector<int>, num_hands> itv_points{};

	/** Holds pattern mod information, one float per interval per mod per hand.
	* In most cases, these are multiplying values, so neutral is 1.
	* For agnostic pattern mods, the values will be the same across hands.
	*/
	std::array<std::array<std::vector<float>, NUM_CalcPatternMod>,
			   num_hands>
	  pmod_vals{};

	/** Holds base calculated difficulties, one float per interval per type per hand.
	* Contains NPS, MSD, Runningman, and Tech related values.
	* Mostly used for its NPSBase values.
	*/
	std::array<std::array<std::vector<float>, NUM_CalcDiffValue>,
			   num_hands>
	  init_base_diff_vals{};

	/** Holds base difficulties adjusted by patternmods. It begins at NPSBase.
	* One float per interval per skillset per hand.
	* Apply stam model to these (but output is sent to stam_adj_diff, not
	* modified here). There are at least two valid reasons to use different
	* base values used for stam, the first being because something that
	* alternates frequently between js and hs (du und ich), and has low
	* detection in each skillset section for the other, will have large stam
	* breaks in both the js and the hs pass, even if it's roughly equivalently
	* stamina draining. This produces a drastically reduced stamina effect for
	* a file that should arguably have a _higher_ stamina tax, due to constant
	* pattern type swapping. This is adjusted for in js/hs. The second is
	* that pattern mods that push down to extreme degrees stuff, like jumptrills
	* or roll walls, will also implicitly push down their effect on stam to
	* to the point where it may be considered a "break", even though it's
	* really not. At best it's a very different kind of stamina drain
	* (trill vs anchor). This is not accounted for in any way, and no
	* estimation is made on how much this actually messes with stuff
	* (could be minor, could be major, could be minor for most files and
	* major for a select few)
	*/
	std::array<std::array<std::vector<float>, NUM_Skillset>,
			   num_hands>
	  base_adj_diff{};

	/** Holds base stamina difficulty values for use in the stamina model.
	* Beginning at NPSBase, these are adjusted slightly by JS and HS related
	* diff adjustments in Calc::InitAdjDiff (the max between adjusted diff and
	* the NPSBase for the skillset)
	* One float per interval per skillset per hand.
	*/
	std::array<std::array<std::vector<float>, NUM_Skillset>,
			   num_hands>
	  base_diff_for_stam_mod{};

	/** Pattern adjusted difficulty. Allocate only once - stam needs to be based
	* on the above, and it needs to be recalculated every time the player_skill
	* value changes, again based on the above. Technically we could use the
	* skill_stamina element of the arrays to store this and save an allocation
	* but that might just be too confusing idk.
	* One float per interval.
	*/
	std::vector<float> stam_adj_diff{};

	/** Jack difficulty, one pair per interval per hand. The pair is for
	* debug purposes (temporarily).
	* The reason for this is that jack_diff is composed of only specific
	* jack difficulty entries that do not correspond to any time. The pair
	* provides timestamps of each difficulty value for debug graphing.
	* The difficulty values in this come from the hardest anchor for the hand.
	*/
	std::array<std::vector<std::pair<float, float>>, num_hands> jack_diff{};

	// number of jacks by hand for intervals
	// std::array<std::vector<int>, num_hands>
	// itv_jack_diff_size{};

	/// unused - formerly populated by jack related point loss values
	std::array<std::vector<float>, num_hands> jack_loss{};

	/// Only used for debugging jack stamina related adjustments to jack_diff
	std::array<std::vector<float>, num_hands> jack_stam_stuff{};

	/** Base tech difficulty per row of current interval being scanned.
	* Composed of the mean of a small moving window of previous tech values.
	* See techyo::advance_base for the intense details ...
	*/
	std::array<float, max_rows_for_single_interval> tc_static{};

	/** Base Chordjack difficulty per row of current interval being scanned.
	* See struct ceejay for the intense details ...
	*/
	std::array<float, max_rows_for_single_interval> cj_static{};

	/// Total number of intervals for the current file/rate (one per half second)
	int numitv = 0;

	/// Total points achievable in the current file (two per note)
	float MaxPoints = 0;

	/// multiplier to resultant roughly determined by a combination
	/// of nps and file length
	float grindscaler = 1.F;

	/** Debug values - mostly patternmod values per interval per hand.
	* These are unnecessary now that the active session calc is a persistent
	* songman singleton, and could/should be removed and the debug values
	* pulled straight from the calc
	*/
	std::array<std::vector<std::vector<std::vector<float>>>, num_hands>
	  debugValues{};
	std::array<std::array<std::vector<float>, NUM_Skillset>, num_hands> debugMSD{};
	std::array<std::array<std::vector<float>, NUM_Skillset>, num_hands> debugPtLoss{};
	std::array<std::array<std::vector<float>, NUM_Skillset>, num_hands> debugTotalPatternMod{};

	/// per hand, per column, vector of pairs of coeff.variance with timestamps
	/// the CVs are based on a moving window
	std::array<std::array<std::vector<std::pair<float, float>>, 2>, num_hands>
	  debugMovingWindowCV{};

	/// per hand vector of arrays: techyo chaos values of [row_time, pewp, obliosis, c]
	std::array<std::vector<std::array<float, 4>>, num_hands> debugTechVals{};

	/** Grow every interval-dependent vector we use.
	* The size could be reduced but there isn't a big need for it.
	* This does nothing if amt < the size of the vectors.
	*/
	void resize_interval_dependent_vectors(size_t amt)
	{
		// there isn't a real need to make our vectors smaller
		if (amt < adj_ni.size())
			return;
		
		// grow each vector
		// resize is used to construct defaults, the space should be used immediately
		adj_ni.resize(amt);
		itv_size.resize(amt);
		for (auto& v : itv_points)
			v.resize(amt);
		for (auto& a : pmod_vals)
			for (auto& v : a)
				v.resize(amt);
		for (auto& a : init_base_diff_vals)
			for (auto& v : a)
				v.resize(amt);
		for (auto& a : base_adj_diff)
			for (auto& v : a)
				v.resize(amt);
		for (auto& a : base_diff_for_stam_mod)
			for (auto& v : a)
				v.resize(amt);
		stam_adj_diff.resize(amt);
	}
};

/// <summary>
/// Calc driving function used for generating score-based skillset values.
/// </summary>
/// <param name="NoteInfo">Output from NoteData::SerializeNoteData2</param>
/// <param name="musicrate">Music rate to adjust NoteInfo row times</param>
/// <param name="goal">Given score value percentage - 1.0 is 100%</param>
/// <param name="calc">Pointer to the Calc instance to use
/// if using threads or a centralized Calc.</param>
/// <returns>A list of the resulting skillset values.</returns>
MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo,
		   float musicrate,
		   float goal,
		   Calc* calc) -> std::vector<float>;
/// <summary>
/// Calc driving function used for generating skillset values for caching.
/// </summary>
/// <param name="NoteInfo">Output from NoteData::SerializeNoteData2</param>
/// <param name="calc">Pointer to the Calc instance to use
/// if using threads or a centralized Calc.</param>
/// <returns>MinaSD, a list of the resulting skillset values,
/// for every rate.</returns>
MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo, Calc* calc) -> MinaSD;
/// <summary>
/// Calc driving function used for generating skillset values for debugging.
/// Works the same as the score-based MinaSDCalc, but runs debug mode.
/// </summary>
/// <param name="NoteInfo">Output from NoteData::SerializeNoteData2</param>
/// <param name="musicrate">Music rate to adjust NoteInfo row times</param>
/// <param name="goal">Given score value percentage - 1.0 is 100%</param>
/// <param name="handInfo">Vector to fill with debugValue info per hand</param>
/// <param name="debugstrings">Vector to fill with a string
/// representation of the notes in every interval</param>
/// <param name="calc">Pointer to the calc instance to use
/// if using threads or a centralized Calc.</param>
MINACALC_API void
MinaSDCalcDebug(
  const std::vector<NoteInfo>& NoteInfo,
  float musicrate,
  float goal,
  std::vector<std::vector<std::vector<std::vector<float>>>>& handInfo,
  std::vector<std::string>& debugstrings,
  Calc& calc);
/// External access to the internally defined calculator version number.
MINACALC_API auto
GetCalcVersion() -> int;
