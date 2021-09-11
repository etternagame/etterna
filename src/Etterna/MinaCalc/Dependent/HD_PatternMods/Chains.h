#pragma once
#include "../MetaIntervalHandInfo.h"
#include "../HD_Sequencers/ChainSequencing.h"

struct ChainsMod
{
	const CalcPatternMod _pmod = Chains;
	const std::string name = "ChainsMod";

#pragma region params

	float min_mod = 1.F;
	float max_mod = 1.1F;

	float anchor_len_weight = .5F;
	float len_scaler = 0.1F;
	float swap_scaler = 0.10775F;

	float base = .75F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "len_scaler", &len_scaler },
		{ "anchor_len_weight", &anchor_len_weight },
		{ "swap_scaler", &swap_scaler },
	};
#pragma endregion params and param map

	Chain_Sequencer chain;
	int max_chain_swaps = 0;
	int max_total_len = 0;
	int max_anchor_len = 0;

	float pmod = neutral;

#pragma region generic functions

	void full_reset()
	{
		chain.zero();

		max_chain_swaps = 0;
		max_total_len = 0;
		max_anchor_len = 0;

		pmod = neutral;
	}

#pragma endregion

	void advance_sequencing(const col_type& ct, const base_type& bt, const col_type& last_ct, const float& any_ms)
	{
		chain(ct, bt, last_ct, any_ms);
	}

	void set_pmod(const metaItvHandInfo& mitvhi)
	{
		const auto& itvhi = mitvhi._itvhi;
		const auto& base_types = mitvhi._base_types;

		// if cur > max when we ended the interval, grab it
		max_chain_swaps = chain.get_max_chain_swaps();
		max_total_len = chain.get_max_total_len();
		max_anchor_len = chain.get_max_anchor_len();

		// nothing here
		if (itvhi.get_taps_nowi() == 0) {
			pmod = neutral;
			return;
		}

		// pmod = base + (sum)
		//	max anchor / total rows * scale
		//	max swaps / total rows * scale

		// an interval with only jacks of 11112222 has no completed seq
		// the clamp should catch that scenario
		// otherwise assume conditions which continue chains are in chains
		auto taps_in_any_sequence = std::clamp(base_types[base_single_single] +
												 base_types[base_single_jump] +
												 base_types[base_jump_single],
											   1,
											   max_total_len);
		auto tapsF = static_cast<float>(taps_in_any_sequence);

		auto csF = static_cast<float>(max_chain_swaps);
		auto clF = static_cast<float>(max_total_len);
		auto caF = static_cast<float>(max_anchor_len);

		// anchor_len_weight should be [0,1]
		// 1 -> worth = entire chain length
		// 0 -> worth = longest anchor length
		auto anchor_len_worth =
		  weighted_average(clF, caF, anchor_len_weight, 1.F);

		auto anchor_worth = fastsqrt(anchor_len_worth / tapsF) * len_scaler;
		auto swap_worth = fastsqrt(csF / tapsF) * swap_scaler;
		pmod = std::clamp(base + anchor_worth + swap_worth, min_mod, max_mod);
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
		chain.reset_max_seq();
		max_chain_swaps = 0;
		max_total_len = 0;
		max_anchor_len = 0;
	}
};
