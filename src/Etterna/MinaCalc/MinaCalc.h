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

using MinaSD = std::vector<std::vector<float>>;

class Calc;

// intervals are half seconds
// this number is a default which should work for most files
// the calc should internally grow the vector if a larger number is needed
static const int default_interval_count = 1000;

// this should allow for anything except for things like 24 hours of stream
// 24 hours of 100 bpm stream takes up around 130k intervals
// this cap will prevent extremely unnecessary huge allocations for end users
static const int max_intervals = 100000;

/* intervals are _half_ second, no point in wasting time or cpu cycles on 100
 * nps joke files. even at most generous, 100 nps spread across all fingers,
 * that's still 25 nps which is considerably faster than anyone can sustain
 * vibro for a full second */
static const int max_rows_for_single_interval = 50;

enum hands
{
	left_hand,
	right_hand,
	num_hands,
};

// this cuts a small amount of redudant calculation at the cost of large memory
// allocation... not sure if worth...
struct RowInfo
{
	unsigned row_notes = 0U;
	int row_count = 0;
	std::array<int, num_hands> hand_counts = { 0, 0 };
	float row_time = 0.F;
};

class Calc
{
  public:
	/* Primary calculator function that wraps everything else. Runs notedata
	 * through ulbu which builds the base diff values and then runs the chisel
	 * function over the produced diff vectors for each skillset. The main
	 * drawback of this approach is files with multiple skillsets tend to get
	 * significantly underrated */
	auto CalcMain(const std::vector<NoteInfo>& NoteInfo,
				  float music_rate,
				  float score_goal) -> std::vector<float>;

	// for music select debug output, should only ever be true at music select
	bool debugmode = false;

	// set to true for scores, false for cache
	bool ssr = true;

  private:
	/* Splits up the chart by each hand and processes them individually to
	 * produce hand specific base difficulty values, which are then passed to
	 * the chisel functions. Hardcode a limit for nps and if we hit it just
	 * return max value for the calc and move on, there's no point in
	 * calculating values for 500 nps joke files. Return value is whether or not
	 * we should skip calculation */
	auto InitializeHands(const std::vector<NoteInfo>& NoteInfo,
						 float music_rate,
						 float offset) -> bool;

	/* Returns estimate of player skill needed to achieve score goal on chart.
	 * The player_skill parameter gives an initial guess and floor for player
	 * skill. Resolution relates to how precise the answer is. Additional
	 * parameters give specific skill sets being tested for.*/
	auto Chisel(float player_skill,
				float resolution,
				float score_goal,
				Skillset ss,
				bool stamina,
				bool debugoutput = false) -> float;

	static inline void InitAdjDiff(Calc& calc, const int& hi);

  public:
	Calc() { resize_interval_dependent_vectors(default_interval_count); }
	
	// the most basic derivations from the most basic notedata
	std::vector<std::array<RowInfo, max_rows_for_single_interval>>
	  adj_ni;

	// size of each interval in rows
	std::vector<int> itv_size{};

	// Point allotment for each interval
	std::array<std::vector<int>, num_hands> itv_points{};

	// holds pattern mods
	std::array<std::array<std::vector<float>, NUM_CalcPatternMod>,
			   num_hands>
	  doot{};

	// Calculated difficulty for each interval
	std::array<std::array<std::vector<float>, NUM_CalcDiffValue>,
			   num_hands>
	  soap{};

	/* Apply stam model to these (but output is sent to stam_adj_diff, not
	 * modified here). There are at least two valid reasons to use different
	 * base values used for stam, the first being because something that
	 * alternates frequently between js and hs (du und ich), and has low
	 * detection in each skillset section for the other, will have large stam
	 * breaks in both the js and the hs pass, even if it's roughly equivalently
	 * stamina draining. This produces a drastically reduced stamina effect for
	 * a file that should arguably have a _higher_ stamina tax, due to constant
	 * pattern type swapping. As of this commit this is adjusted for in js/hs.
	 * The second is that pattern mods that push down to extreme degrees stuff
	 * like jumptrills, or roll walls, will also implicitly push down their
	 * effect on stam to the point where it may be considered a "break", even
	 * though it's really not. At best it's a very different kind of stamina
	 * drain (trill vs anchor). As of this commit this is not accounted for in
	 * any way, and no estimation is made on how much this actually messes with
	 * stuff (could be minor, could be major, could be minor for most files and
	 * major for a select few) */
	std::array<std::array<std::vector<float>, NUM_Skillset>,
			   num_hands>
	  base_adj_diff{};

	// input that the stamina model uses to apply to the base diff
	std::array<std::array<std::vector<float>, NUM_Skillset>,
			   num_hands>
	  base_diff_for_stam_mod{};

	/* Pattern adjusted difficulty, allocate only once, stam needs to be based
	 * on the above, and it needs to be recalculated every time the player_skill
	 * value changes, again based on the above, technically we could use the
	 * skill_stamina element of the arrays to store this and save an allocation
	 * but that might just be too confusing idk */
	std::vector<float> stam_adj_diff{};

	// a vector of {row_time, diff} for each hand
	std::array<std::vector<std::pair<float, float>>, num_hands> jack_diff{};

	// number of jacks by hand for intervals
	// std::array<std::vector<int>, num_hands>
	// itv_jack_diff_size{};

	std::array<std::vector<float>, num_hands> jack_loss{};

	// jack stam stuff for debug
	std::array<std::vector<float>, num_hands> jack_stam_stuff;

	// base techdifficulty per row of current interval being scanned
	std::array<float, max_rows_for_single_interval> tc_static{};

	// total number of intervals for the current file/rate
	int numitv = 0;

	// Total points achievable in the current file
	float MaxPoints = 0;

	/* these are unnecessary now that the active session calc is a persistent
	 * songman singleton, and could/should be removed and the debug values
	 * pulled straight from the calc */
	std::array<std::vector<std::vector<std::vector<float>>>, num_hands>
	  debugValues;

	// grow every interval-dependent vector we use
	// the size could be reduced but there isn't a big need for it
	// this does nothing if amt < the size of the vectors
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
		for (auto& a : doot)
			for (auto& v : a)
				v.resize(amt);
		for (auto& a : soap)
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

MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo,
		   float musicrate,
		   float goal,
		   Calc* calc) -> std::vector<float>;
MINACALC_API auto
MinaSDCalc(const std::vector<NoteInfo>& NoteInfo, Calc* calc) -> MinaSD;
MINACALC_API void
MinaSDCalcDebug(
  const std::vector<NoteInfo>& NoteInfo,
  float musicrate,
  float goal,
  std::vector<std::vector<std::vector<std::vector<float>>>>& handInfo,
  std::vector<std::string>& debugstrings,
  Calc& calc);
MINACALC_API auto
GetCalcVersion() -> int;
