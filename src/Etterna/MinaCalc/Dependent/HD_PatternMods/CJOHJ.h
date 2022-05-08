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

	float min_mod = 0.5F;
	float max_mod = 1.F;

	float max_seq_weight = 0.65F;
	float max_seq_pool = 1.2F;
	float max_seq_scaler = 0.8F;

	float prop_pool = 1.4F;
	float prop_scaler = 1.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "max_seq_weight", &max_seq_weight },
		{ "max_seq_pool", &max_seq_pool },
		{ "max_seq_scaler", &max_seq_scaler },

		{ "prop_pool", &prop_pool },
		{ "prop_scaler", &prop_scaler },
	};
#pragma endregion params and param map

	OHJ_Sequencer ohj;
	int max_ohjump_seq_taps = 0;
	int cc_taps = 0;

	float floatymcfloatface = 0.F;
	// number of jumps scaled to total taps in hand
	float base_seq_prop = 0.F;
	// size of sequence scaled to total taps in hand
	float base_jump_prop = 0.F;

	float max_seq_component = neutral;
	float prop_component = neutral;
	float pmod = neutral;

#pragma region generic functions

	void full_reset()
	{
		ohj.zero();

		max_ohjump_seq_taps = 0;
		cc_taps = 0;

		floatymcfloatface = 0.F;
		base_seq_prop = 0.F;
		base_jump_prop = 0.F;

		max_seq_component = neutral;
		prop_component = neutral;
		pmod = neutral;
	}

#pragma endregion

	void advance_sequencing(const col_type& ct, const base_type& bt)
	{
		ohj(ct, bt);
	}

	// build component based on max sequence relative to hand taps
	void set_max_seq_comp()
	{
		max_seq_component = max_seq_pool - (base_seq_prop * max_seq_scaler);
		max_seq_component = max_seq_component < 0.1F ? 0.1F : max_seq_component;
		max_seq_component = fastsqrt(max_seq_component);
	}

	// build component based on number of jumps relative to hand taps
	void set_prop_comp()
	{
		prop_component = prop_pool - (base_jump_prop * prop_scaler);
		prop_component = prop_component < 0.1F ? 0.1F : prop_component;
		prop_component = fastsqrt(prop_component);
	}

	void set_pmod(const metaItvHandInfo& mitvhi)
	{
		const auto& itvhi = mitvhi._itvhi;

		cc_taps = mitvhi._base_types[base_left_right] +
				  mitvhi._base_types[base_right_left];

		assert(cc_taps >= 0);

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

		/* prop scaling only case */

		if (max_ohjump_seq_taps < 3) {

			// need to set now
			base_jump_prop =
			  itvhi.get_col_taps_nowf(col_ohjump) / itvhi.get_taps_nowf();
			set_prop_comp();

			pmod = std::clamp(prop_component, min_mod, max_mod);
			return;
		}

		/* seq scaling only case */

		if (cc_taps == 0) {

			floatymcfloatface = static_cast<float>(max_ohjump_seq_taps);
			base_seq_prop = floatymcfloatface / itvhi.get_taps_nowf();
			set_max_seq_comp();

			pmod = std::clamp(max_seq_component, min_mod, max_mod);
			return;
		}

		/* case optimization end */

		floatymcfloatface = static_cast<float>(max_ohjump_seq_taps);
		base_seq_prop = floatymcfloatface / mitvhi._itvhi.get_taps_nowf();
		set_max_seq_comp();
		max_seq_component = std::clamp(max_seq_component, 0.1F, max_mod);

		base_jump_prop =
		  itvhi.get_col_taps_nowf(col_ohjump) / itvhi.get_taps_nowf();
		set_prop_comp();
		prop_component = std::clamp(prop_component, 0.1F, max_mod);

		pmod = weighted_average(
		  max_seq_component, prop_component, max_seq_weight, 1.F);
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
		cc_taps = 0;
		ohj.max_seq_taps = 0;
		max_ohjump_seq_taps = 0;
	}
};
