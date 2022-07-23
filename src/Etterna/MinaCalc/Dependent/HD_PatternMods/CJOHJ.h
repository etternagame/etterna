#pragma once
#include "../IntervalHandInfo.h"
#include "../HD_Sequencers/OHJSequencing.h"

/// Hand-Dependent PatternMod detecting one hand jumps.
/// This is used specifically for Chordjacks.
/// initially copied from ohj but the logic should probably be adjusted on
/// multiple levels
struct CJOHJumpMod
{
	const CalcPatternMod _pmod = CJOHJump;
	const std::string name = "CJOHJumpMod";

#pragma region params

	float min_mod = 0.57F;
	float max_mod = 1.F;

	float prop_pool = 1.F;
	float prop_scaler = 0.63F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "prop_pool", &prop_pool },
		{ "prop_scaler", &prop_scaler },
	};
#pragma endregion params and param map

	OHJ_Sequencer ohj;
	int max_ohjump_seq_taps = 0;

	// number of jumps scaled to total taps in hand
	float base_seq_prop = 0.F;
	// size of sequence scaled to total taps in hand
	float base_jump_prop = 0.F;

	float prop_component = neutral;
	float pmod = neutral;

#pragma region generic functions

	void full_reset()
	{
		ohj.zero();

		max_ohjump_seq_taps = 0;

		base_seq_prop = 0.F;
		base_jump_prop = 0.F;

		prop_component = neutral;
		pmod = neutral;
	}

#pragma endregion

	void advance_sequencing(const col_type& ct, const base_type& bt)
	{
		ohj(ct, bt);
	}

	// build component based on number of jumps relative to hand taps
	void set_prop_comp()
	{
		prop_component = prop_pool - (base_jump_prop * prop_scaler);
		prop_component = prop_component < 0.1F ? 0.1F : prop_component;
	}

	void set_pmod(const metaItvHandInfo& mitvhi)
	{
		const auto& itvhi = mitvhi._itvhi;

		// if cur_seq > max when we ended the interval, grab it
		max_ohjump_seq_taps = ohj.cur_seq_taps > ohj.max_seq_taps
								? ohj.cur_seq_taps
								: ohj.max_seq_taps;

		/* case optimization start */

		// nothing here or there are no ohjumps
		if (itvhi.get_taps_nowi() == 0 ||
			itvhi.get_col_taps_nowi(col_ohjump) == 0) {
			pmod = neutral;
			return;
		}

		// everything in the interval is in an ohj sequence
		if (max_ohjump_seq_taps >= itvhi.get_taps_nowi()) {
			pmod = min_mod;
			return;
		}

		// floats for less casting
		// these should always be whole numbers
		const auto ohjcount = itvhi.get_col_taps_nowf(col_ohjump) / 2.f;
		const auto tapcount = (itvhi.get_col_taps_nowf(col_left) - ohjcount) +
							  (itvhi.get_col_taps_nowf(col_right) - ohjcount);
		const auto rows = ohjcount + tapcount;

		base_jump_prop = ohjcount / rows;
		set_prop_comp();
		prop_component = std::clamp(prop_component, 0.1F, max_mod);

		pmod = prop_component;
		pmod = std::clamp(pmod, min_mod, max_mod);
	}

	auto operator()(const metaItvHandInfo& mitvhi) -> float
	{
		set_pmod(mitvhi);

		interval_end();
		return pmod;
	}

	void interval_end()
	{
		// reset any interval stuff here
		ohj.max_seq_taps = 0;
		max_ohjump_seq_taps = 0;
	}
};
