#pragma once
#include "../IntervalHandInfo.h"

/// Hand-Dependent PatternMod detecting minijacks.
/// Minijacks cannot lead into minijacks.
/// Minijacks are simply cases of 11 or 22.
struct MinijackMod
{
	const CalcPatternMod _pmod = Minijack;
	const std::string name = "MinijackMod";

#pragma region params

	float min_mod = 1.F;
	float max_mod = 1.2F;
	float base = 0.5F;

	float mj_scaler = 2.5F;
	float mj_buffer = 4.5F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "minijack_scaler", &mj_scaler },
		{ "minijack_buffer", &mj_buffer },
	};
#pragma endregion params and param map

	// ms times for each finger
	CalcMovingWindow<float> left_ms{};
	CalcMovingWindow<float> right_ms{};

	// 2.0 is equivalent to 8th -> 16th
	// aka a 16th minijack in 16th js
	// if the min of the window is 
	const float minijack_speed_increase_factor = 1.9F;

	// if the ms gap after a minijack larger by this factor
	// then the ms gap for the minijack is confirmed a minijack
	const float minijack_confirmation_factor = 1.3F;

	// throw out data if this many ms pass
	const float dont_care_threshold = 500.F;

	// 150ms is a 100 bpm 16th
	const float slow_minijack_cutoff_ms = 149.5F;

	const int window = 3;
	int minijacks = 0;
	float pmod = min_mod;

#pragma region generic functions

	void full_reset()
	{
		pmod = neutral;
		minijacks = 0;
		left_ms.fill(ms_init);
		right_ms.fill(ms_init);
	}

	void reset_mw_for_ct(const col_type& ct) {
		switch (ct) {
			case col_left: {
				left_ms.fill(ms_init);
				break;
			}
			case col_right: {
				right_ms.fill(ms_init);
				break;
			}
			case col_ohjump: {
				left_ms.fill(ms_init);
				right_ms.fill(ms_init);
				break;
			}
			default:
				break;
		}
	}

	void setup()
	{
		
	}

#pragma endregion

	void minijack_check(const CalcMovingWindow<float>& mv) {
		// make sure the window is filled out
		const auto max = mv.get_max_for_window(window);
		if (max != ms_init) {
			auto i = max_moving_window_size;
			const auto min = mv.get_min_for_window(window);
			const auto recent_ms = mv[--i];
			const auto last_ms = mv[--i];
			const auto last_last_ms = mv[--i];

			// basically we dont care if the "minijack" exists
			// if it is so slow
			if (last_ms > slow_minijack_cutoff_ms) {
				return;
			}

			// for a minijack to count
			// it must have a gap before it and after
			// the gap before : speedup of basically 8th -> 16th or faster
			// the gap after : slowdown of basically 16th -> 12th or slower
			if (last_ms == min &&
				recent_ms > last_ms * minijack_confirmation_factor &&
				last_last_ms > last_ms * minijack_speed_increase_factor) {
				minijacks++;
			}
		}
	}

	void advance_sequencing(const col_type& ct,
							const float& ms_now)
	{
		/*
		if (ms_now > dont_care_threshold) {
			// data has become stale
			reset_mw_for_ct(ct);
			return;
		}
		*/

		switch (ct) {
			case col_left: {
				left_ms(ms_now);
				minijack_check(left_ms);
				break;
			}
			case col_right: {
				right_ms(ms_now);
				minijack_check(right_ms);
				break;
			}
			case col_ohjump: {
				left_ms(ms_now);
				right_ms(ms_now);
				minijack_check(left_ms);
				minijack_check(right_ms);
				break;
			}
			default:
				break;
		}
	}

	void set_pmod(const ItvHandInfo& itvhi)
	{
		if (minijacks == 0 || itvhi.get_taps_nowi() == 0) {
			pmod = neutral;
			return;
		}

		const auto mj = (static_cast<float>(minijacks) + mj_buffer) * mj_scaler;
		const auto taps = itvhi.get_taps_nowf() - mj_buffer;

		pmod = base + mj / taps;


		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const ItvHandInfo& itvhi) -> float
	{
		set_pmod(itvhi);

		interval_end();
		return pmod;
	}

	void interval_end()
	{
		minijacks = 0;
	}
};
