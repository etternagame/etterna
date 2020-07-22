#pragma once
#include "../IntervalHandInfo.h"

/* this is complex enough it should probably have its own sequencer, there's
 * also a fair bit of redundancy between this, wrjt, wrr. Currently this has the
 * best component construction for the pmod so maybe the other mods should use
 * this as a template? */

static const int max_trills_per_interval = 4;

// almost identical to wrr, refer to comments there
struct OHTrillMod
{
	const CalcPatternMod _pmod = OHTrill;
	const std::string name = "OHTrillMod";

#pragma region params

	float window_param = 3.F;

	float min_mod = 0.9F;
	float max_mod = 1.F;
	float base = 1.35F;
	float suppression = 0.4F;

	float cv_reset = 1.F;
	float cv_threshhold = 0.5F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
		{ "suppression", &suppression },

		{ "cv_reset", &cv_reset },
		{ "cv_threshhold", &cv_threshhold },
	};
#pragma endregion params and param map

	int window = 0;
	int cc_window = 0;

	bool luca_turilli = false;

	// ok new plan, ohj, wrjt and wrr are relatively well tuned so i'll try this
	// here, handle merging multiple sequences in a single interval into one
	// value at interval end and keep a window of that. suppose we have two
	// intervals of 12 notes with 8 in trill formation, one has an 8 note trill
	// and the other has two 4 note trills at the start/end, we want to punish
	// the 8 note trill harder, this means we _will_ be resetting the
	// consecutive trill counter every interval, but will not be resetting the
	// trilling flag, this way we don't have to futz around with awkward
	// proportion math, similar to thing 1 and thing 2
	CalcMovingWindow<float> badjuju;
	CalcMovingWindow<int> _mw_oht_taps;

	std::array<int, max_trills_per_interval> foundyatrills = { 0, 0, 0, 0 };

	int found_oht = 0;
	int oht_len = 0;
	int oht_taps = 0;

	float hello_my_name_is_goat = 0.F;

	float moving_cv = cv_reset;
	float pmod = min_mod;

#pragma region generic functions

	void full_reset()
	{
		badjuju.zero();

		luca_turilli = false;
		found_oht = 0;
		oht_len = 0;

		for (auto& v : foundyatrills) {
			v = 0;
		}

		moving_cv = cv_reset;
		pmod = neutral;
	}

	void setup()
	{
		window =
		  std::clamp(static_cast<int>(window_param), 1, max_moving_window_size);
		cc_window =
		  std::clamp(static_cast<int>(window_param), 1, max_moving_window_size);
	}

#pragma endregion

	auto make_thing(const float& itv_taps) -> float
	{
		hello_my_name_is_goat = 0.F;

		if (found_oht == 0) {
			return 0.F;
		}

		for (auto& v : foundyatrills) {
			if (v == 0) {
				continue;
			}

			// water down smaller sequences
			hello_my_name_is_goat =
			  (static_cast<float>(v) / itv_taps) - suppression;
		}
		return std::clamp(hello_my_name_is_goat, 0.1F, 1.F);
	}

	void complete_seq()
	{
		if (!luca_turilli || oht_len == 0) {
			return;
		}

		if (found_oht < max_trills_per_interval) {
			foundyatrills.at(found_oht) = oht_len;
		}

		luca_turilli = false;
		oht_len = 0;
		++found_oht;
		moving_cv = (moving_cv + cv_reset) / 2.F;
	}

	auto oht_timing_check(const CalcMovingWindow<float>& ms_any) -> bool
	{
		moving_cv = (moving_cv + ms_any.get_cv_of_window(cc_window)) / 2.F;
		// the primary difference from wrr, just check cv on the base ms values,
		// we are looking for values that are all close together without any
		// manipulation
		return moving_cv < cv_threshhold;
	}

	void wifflewaffle()
	{
		if (luca_turilli) {
			++oht_len;
			++oht_taps;
		} else {
			luca_turilli = true;
			oht_len += 3;
			oht_taps += 3;
		}
	}

	void advance_sequencing(const meta_type& mt,
							const CalcMovingWindow<float>& ms_any)
	{

		switch (mt) {
			case meta_cccccc:
				if (oht_timing_check(ms_any)) {
					wifflewaffle();
				} else {
					complete_seq();
				}
				break;
			case meta_ccacc:
				// wait to see what happens
				break;
			case meta_enigma:
			case meta_meta_enigma:
				// also wait to see what happens, but not if last was ccacc,
				// since we only don't complete there if we don't immediately go
				// back into ohts

				// this seems to be overkill with how lose the detection is
				// already anyway

				// if (now.last_cc == meta_ccacc) {
				//	complete_seq();
				//}
				// break;
			default:
				complete_seq();
				break;
		}
	}

	void set_pmod(const ItvHandInfo& itvhi)
	{

		// no taps, no trills
		if (itvhi.get_taps_windowi(window) == 0 ||
			_mw_oht_taps.get_total_for_window(window) == 0) {
			pmod = neutral;
			return;
		}

		// full oht
		if (itvhi.get_taps_windowi(window) ==
			_mw_oht_taps.get_total_for_window(window)) {
			pmod = min_mod;
			return;
		}

		badjuju(make_thing(itvhi.get_taps_nowf()));

		pmod = base - badjuju.get_mean_of_window(window);
		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		if (oht_len > 0 && found_oht < max_trills_per_interval) {
			foundyatrills.at(found_oht) = oht_len;
			++found_oht;
		}

		_mw_oht_taps(oht_taps);

		set_pmod(itvhi);

		interval_end();
		return pmod;
	}

	void interval_end()
	{
		foundyatrills.fill(0);
		found_oht = 0;
		oht_len = 0;
		oht_taps = 0;
	}
};
