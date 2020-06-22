#pragma once
#include <array>
#include <vector>

#include "Etterna/Globals/MinaCalc/SequencingHelpers.h"

/* MS difficulty bases are going to be sequence constructed row by row here, the
 * nps base may be moved here later but not right now. we'll use statically
 * allocated arrays to built difficulty for each interval, and the output will
 * be placed into statically allocated arrays for base difficulties. */

// we've already thrown out anything that exceeeds max_rows_for_single_interval
// before ever hitting here, so this should be safe
static thread_local std::array<float, max_rows_for_single_interval> jk_static;
static thread_local std::array<float, max_rows_for_single_interval> tc_static;

/*
// Calculated difficulty for each interval
static thread_local std::array<
  std::array<std::array<float, max_intervals>, NUM_CalcDiffValue>,
  num_hands>
  soap;

// not necessarily self extraplanetary
// apply stam model to these (but output is sent to stam_adj_diff, not modified
// here)
static thread_local std::
  array<std::array<std::array<float, max_intervals>, NUM_Skillset>, num_hands>
	base_adj_diff;
// but use these as the input for model
static thread_local std::
  array<std::array<std::array<float, max_intervals>, NUM_Skillset>, num_hands>
	base_diff_for_stam_mod;

// pattern adjusted difficulty, allocate only once, stam needs to be based
// on the above, and it needs to be recalculated every time the player_skill
// value changes, again based on the above, technically we could use the
// skill_stamina element of the arrays to store this and save an allocation
// but that might just be too confusing idk
static thread_local std::array<float, max_intervals> stam_adj_diff;

*/
/* NOTE: all _incoming_ diffs should be stored as MS values, and only converted
 * to scaled NPS on the way out */

struct vribbit
{
	// ok this is a little funky, we'll track the interval average jack speed
	// difficulty, and also the interval max, then average the two. functionally
	// this will give shorter jacks a bit of a boost
	inline void advance_base(const float& jk_diff)
	{
		jk_itv_ms_min = min(jk_itv_ms_min, jk_diff);
		advance_jk_comp(jk_diff);
		++row_counter;
	}

	// final output difficulty for this interval
	[[nodiscard]] inline auto get_itv_diff() const -> float
	{
		if (row_counter == 0) {
			return 0.F;
		}

		float ms_total = 0.F;
		for (int i = 0; i < row_counter; ++i) {
			ms_total += jk_static.at(i);
		}

		float ms_mean = ms_total / static_cast<float>(row_counter);
		return ms_to_scaled_nps((ms_mean + jk_itv_ms_min) / 2.F);
	}

	inline void interval_end()
	{
		row_counter = 0;
		jk_itv_ms_min = 0.F;
	}

  private:
	int row_counter = 0;
	float jk_itv_ms_min = 0.F;

	inline void advance_jk_comp(const float& jk_diff)
	{
		if (row_counter >= max_rows_for_single_interval) {
			return;
		}

		jk_static.at(row_counter) = jk_diff;
		jk_itv_ms_min = max(jk_itv_ms_min, jk_diff);
	}
};

struct techyo
{
	// if this looks ridiculous, that's because it is
	inline void advance_base(const SequencerGeneral& seq, const col_type& ct)
	{
		if (row_counter >= max_rows_for_single_interval) {
			return;
		}

		float a = seq.get_sc_ms_now(ct);
		float b = ms_init;
		if (ct == col_ohjump) {
			b = seq.get_sc_ms_now(ct, false);
		} else {
			b = seq.get_cc_ms_now();
		}

		float c = fastsqrt(a) * fastsqrt(b);

		float pineapple = seq._mw_any_ms.get_cv_of_window(4);
		float porcupine = seq._mw_sc_ms[col_left].get_cv_of_window(4);
		float sequins = seq._mw_sc_ms[col_right].get_cv_of_window(4);
		float oioi = 0.5F;
		pineapple = CalcClamp(pineapple + oioi, oioi, 1.F + oioi);
		porcupine = CalcClamp(porcupine + oioi, oioi, 1.F + oioi);
		sequins = CalcClamp(sequins + oioi, oioi, 1.F + oioi);

		float scoliosis = seq._mw_sc_ms[col_left].get_now();
		float poliosis = seq._mw_sc_ms[col_right].get_now();
		float obliosis = 0.F;
		if (ct == col_left) {

			obliosis = poliosis / scoliosis;

		} else {

			obliosis = scoliosis / poliosis;
		}
		obliosis = CalcClamp(obliosis, 1.f, 10.F);
		float pewp = fastsqrt(div_high_by_low(scoliosis, poliosis) - 1.F);

		pewp /= obliosis;
		float vertebrae = CalcClamp(
		  ((pineapple + porcupine + sequins) / 3.F) + pewp, oioi, 1.F + oioi);

		teehee(c / vertebrae);

		tc_static.at(row_counter) = teehee.get_mean_of_window(2);
		++row_counter;
	}

	inline void advance_rm_comp(const float& rm_diff)
	{
		rm_itv_max_diff = max(rm_itv_max_diff, rm_diff);
	}

	// final output difficulty for this interval, merges base diff, runningman
	// anchor diff
	[[nodiscard]] inline auto get_itv_diff(const float& nps_base) const -> float
	{
		// for now do simple thing, for this interval either use the higher
		// between weighted adjusted ms/nps base and runningman diff
		// we definitely don't want to average here because we don't want tech
		// to only be files with strong runningman pattern detection, but we
		// could probably do something more robust at some point
		return max(weighted_average(get_tc_base(), nps_base, 5.5F, 9.F),
				   rm_itv_max_diff);
	}

	inline void interval_end()
	{
		row_counter = 0;
		rm_itv_max_diff = 0.F;
	}

  private:
	// how many non-empty rows have we seen
	int row_counter = 0;

	// jank stuff.. keep a small moving average of the base diff
	CalcMovingWindow<float> teehee;

	// max value of rm diff for this interval, this will be an exception to the
	// only storing ms rule, rm diff will be stored as a pre-converted diff
	// value because rm_sequencing may adjust the scaled diff using the rm
	// components in ways we can't emulate here (nor should we try)
	float rm_itv_max_diff = 0.F;

	// get the interval base diff, which will then be merged via weighted
	// average with npsbase, and then compared to max_rm diff
	[[nodiscard]] inline auto get_tc_base() const -> float
	{
		if (row_counter == 0) {
			return 0.F;
		}

		float ms_total = 0.F;
		for (int i = 0; i < row_counter; ++i) {
			ms_total += tc_static.at(i);
		}

		float ms_mean = ms_total / static_cast<float>(row_counter);
		return ms_to_scaled_nps(ms_mean);
	}
};

struct diffz
{
	vribbit _jk;
	techyo _tc;

	inline void interval_end()
	{
		_jk.interval_end();
		_tc.interval_end();
	}

	inline void full_reset()
	{
		interval_end();
	}
};
