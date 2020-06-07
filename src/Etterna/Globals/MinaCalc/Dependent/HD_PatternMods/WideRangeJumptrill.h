#pragma once
#include <string>
#include <array>
#include <vector>

#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "Etterna/Globals/MinaCalc/Dependent/IntervalHandInfo.h"

using std::pair;
using std::vector;

// big brain stuff
static const float wrjt_cv_factor = 3.F;

// should update detection so it's more similar to updated wrr
// probably needs better debugoutput
struct WideRangeJumptrillMod
{
	const CalcPatternMod _pmod = { WideRangeJumptrill };
	const std::string name = "WideRangeJumptrillMod";

#pragma region params

	float window_param = 3.F;

	float min_mod = 0.25F;
	float max_mod = 1.F;
	float base = 0.4F;

	float cv_reset = 0.5F;
	float cv_threshhold = 0.15F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "cv_reset", &cv_reset },
		{ "cv_threshhold", &cv_threshhold },
	};
#pragma endregion params and param map

	int window = 0;
	CalcMovingWindow<int> _mw_jt;
	int jt_counter = 0;

	bool bro_is_this_file_for_real = false;
	bool last_passed_check = false;

	float pmod = neutral;

	std::array<float, 3> seq_ms = { 0.F, 0.F, 0.F };

	// put this back again? seems to work well for wrr, however wrr is already
	// more generalized anyway
	// float moving_cv = moving_cv_init;

#pragma region generic functions

	inline void full_reset()
	{
		_mw_jt.zero();
		jt_counter = 0;
		seq_ms.fill(0.f);

		bro_is_this_file_for_real = false;
		last_passed_check = false;
		pmod = neutral;
	}

	inline void setup()
	{
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
	}

#pragma endregion

	inline auto handle_ccacc_timing_check() -> bool
	{
		// we don't want to suppress actual streams that use this pattern, so we
		// will keep a fairly tight requirement on the ms variance

		// we are currently assuming we have xyyx always, and are not interested
		// in xxyy as a part of xxyyxxyyxx transitions, given these conditions
		// we can do some pretty neat stuff

		// seq_ms 0 and 2 will both be cross column ms values, or left->right /
		// right->left, seq_ms 1 will always be an anchor value, so,
		// right->right for example. our interest is in hard nerfing long chains
		// of xyyx patterns that won't get picked up by any of the roll scalers
		// or balance scalers, but are still jumptrillable, for this condition
		// to be true the anchor ms length has to be within a certain ratio of
		// the cross column ms lengths, enabling the cross columns to be hit
		// like jumptrilled flams, the optimal ratio for inflating nps is about
		// 3:1, this is short enough that the nps boost is still high, but long
		// enough that it doesn't become endless minijacking on the anchor
		// given these conditions we can divide seq_ms[1] by 3 and cv check the
		// array, with 2 identical values cc values, even if the anchor ratio
		// floats between 2:1 and 4:1 the cv should still be below 0.25, which
		// is a sensible cutoff that should avoid punishing happenstances of
		// this pattern in just regular files

		seq_ms[1] /= wrjt_cv_factor;
		last_passed_check = cv(seq_ms) < cv_threshhold;
		seq_ms[1] *= wrjt_cv_factor;

		return last_passed_check;
	}

	inline auto handle_acca_timing_check() -> bool
	{
		seq_ms[1] *= wrjt_cv_factor;
		last_passed_check = cv(seq_ms) < cv_threshhold;
		seq_ms[1] /= wrjt_cv_factor;

		return last_passed_check;
	}

	inline auto handle_roll_timing_check() -> bool
	{
		// see ccacc timing check in wrjt for explanations, it's basically the
		// same but we have to invert the multiplication depending on which
		// value is higher between seq_ms[0] and seq_ms[1] (easiest to dummy up
		// a roll in an editor to see why)

		// multiply seq_ms[1] by 3 for the cv check, then put it back so it
		// doesn't interfere with the next round
		if (seq_ms[0] > seq_ms[1]) {
			seq_ms[1] *= wrjt_cv_factor;
			last_passed_check = cv(seq_ms) < cv_threshhold;
			seq_ms[1] /= wrjt_cv_factor;
			return last_passed_check;
		}
		// same thing but divide
		seq_ms[1] /= wrjt_cv_factor;
		last_passed_check = cv(seq_ms) < cv_threshhold;
		seq_ms[1] *= wrjt_cv_factor;
		return last_passed_check;
	}

	inline void update_seq_ms(const base_pattern_type& bt, const float& any_ms, const float& tc_ms)
	{
		seq_ms[0] = seq_ms[1]; // last_last
		seq_ms[1] = seq_ms[2]; // last

		// update now
		// for anchors, track tc_ms
		if (bt == base_single_single) {
			seq_ms[2] = tc_ms;
			// for base_left_right or base_right_left, track cc_ms
		} else {
			seq_ms[2] = any_ms;
		}
	}

	inline auto check_last_mt(const meta_type& mt) -> bool
	{
		if (mt == meta_acca || mt == meta_ccacc || mt == meta_cccccc) {
			if (last_passed_check) {
				return true;
			}
		}
		return false;
	}

	inline void bibblybop(const meta_type& mt)
	{
		++jt_counter;
		if (bro_is_this_file_for_real) {
			++jt_counter;
		}
		if (check_last_mt(mt)) {
			++jt_counter;
			bro_is_this_file_for_real = true;
		}
	}

	inline void advance_sequencing(base_pattern_type& bt,
								   const meta_type& mt,
								   const meta_type& last_mt,
								   const float& any_ms,
								   const float& tc_ms)
	{
		// ignore if we hit a jump
		if (bt == base_jump_jump || bt == base_single_jump) {
			return;
		}

		update_seq_ms(bt, any_ms, tc_ms);

		// look for stuff thats jumptrillyable.. if that stuff... then leads
		// into more stuff.. that is jumptrillyable... then .... badonk it

		switch (mt) {
			case meta_cccccc:
				if (handle_roll_timing_check()) {
					bibblybop(last_mt);
					return;
				}
				break;
			case meta_ccacc:
				if (handle_ccacc_timing_check()) {
					bibblybop(last_mt);
					return;
				}
				break;
			case meta_acca:
				// don't bother adding if the ms values look benign
				if (handle_acca_timing_check()) {
					bibblybop(last_mt);
					return;
				}
				break;
			default:
				break;
		}

		bro_is_this_file_for_real = false;
	}

	inline auto operator()(const ItvHandInfo& itvhi,
						   vector<float> doot[],
						   const int& i) -> float
	{
		_mw_jt(jt_counter);

		// no taps, no jt
		if (itvhi.get_taps_windowi(window) == 0 ||
			_mw_jt.get_total_for_window(window) == 0) {
			return neutral;
		}

		pmod =
		  itvhi.get_taps_windowf(window) / _mw_jt.get_total_for_windowf(window);

		pmod = CalcClamp(pmod, min_mod, max_mod);
		doot[_pmod][i] = pmod;

		interval_reset();
	}

	inline void interval_reset()
	{
		// we could count these in metanoteinfo but let's do it here for now,
		// reset every interval when finished
		jt_counter = 0;
	}
};
