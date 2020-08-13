#pragma once
#include "../IntervalHandInfo.h"

/* retuned oht mod focus tuned to for catching vibro trills like bagatelle */

static const int max_vtrills_per_interval = 4;

// almost identical to wrr, refer to comments there
struct VOHTrillMod
{
	const CalcPatternMod _pmod = VOHTrill;
	const std::string name = "VOHTrillMod";

#pragma region params

	float window_param = 2.F;

	float min_mod = 0.25F;
	float max_mod = 1.F;
	float base = 1.5F;
	float suppression = 0.2F;

	float cv_reset = 1.F;
	float cv_threshhold = 0.25F;

	float min_len = 8.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
		{ "suppression", &suppression },

		{ "cv_reset", &cv_reset },
		{ "cv_threshhold", &cv_threshhold },

		{ "min_len", &min_len },
	};
#pragma endregion params and param map

	int window = 0;
	int cc_window = 0;

	bool luca_turilli = false;

	CalcMovingWindow<float> badjuju;
	CalcMovingWindow<int> _mw_oht_taps;

	std::array<int, max_vtrills_per_interval> foundyatrills = { 0, 0, 0, 0 };

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

		if (found_oht < max_vtrills_per_interval) {
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

		if (_mw_oht_taps.get_total_for_window(window) < min_len) {
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
		if (oht_len > 0 && found_oht < max_vtrills_per_interval) {
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
