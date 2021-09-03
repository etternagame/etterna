#pragma once
#include "../MetaIntervalHandInfo.h"
#include "../HD_Sequencers/ChainSequencing.h"

struct ChainsMod
{
	const CalcPatternMod _pmod = Chains;
	const std::string name = "ChainsMod";

#pragma region params

	float min_mod = 0.7F;
	float max_mod = 1.2F;

	float max_seq_weight = 1.F;
	float max_seq_pool = 1.F;
	float max_seq_scaler = 1.F;

	float chain_swap_pool = 1.F;
	float chain_swap_scaler = 1.F;

	float prop_pool = 1.F;
	float prop_scaler = 1.F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },

		{ "max_seq_weight", &max_seq_weight },
		{ "max_seq_pool", &max_seq_pool },
		{ "max_seq_scaler", &max_seq_scaler },

		{ "chain_swap_pool", &chain_swap_pool },
		{ "chain_swap_scaler", &chain_swap_scaler },

		{ "prop_pool", &prop_pool },
		{ "prop_scaler", &prop_scaler },
	};
#pragma endregion params and param map

	Chain_Sequencer chain;
	int max_chain_single_taps = 0;
	int max_chain_ohj = 0;
	int max_chain_len = 0;
	int max_chain_swaps = 0;
	int cc_taps = 0;

	// combination of max components of sequencer
	float base_seq_prop = 0.F;
	// size of sequence scaled to total taps in hand
	float base_size_prop = 0.F;

	float max_seq_component = neutral;
	float prop_component = neutral;
	float pmod = neutral;

#pragma region generic functions

	void full_reset()
	{
		chain.zero();

		max_chain_single_taps = 0;
		max_chain_ohj = 0;
		max_chain_len = 0;
		max_chain_swaps = 0;
		cc_taps = 0;

		base_seq_prop = 0.F;
		base_size_prop = 0.F;

		max_seq_component = neutral;
		prop_component = neutral;
		pmod = neutral;
	}

#pragma endregion

	void advance_sequencing(const col_type& ct, const base_type& bt, const col_type& last_ct)
	{
		chain(ct, bt, last_ct);
	}

	// build component based on max sequence relative to hand taps
	void set_max_seq_comp()
	{
		max_seq_component = max_seq_pool - (base_seq_prop * max_seq_scaler);
		max_seq_component = max_seq_component < 0.1F ? 0.1F : max_seq_component;
		max_seq_component = fastsqrt(max_seq_component);
	}

	// build component based on number of taps in any chain sequence
	// relative to hand taps
	void set_prop_comp()
	{
		prop_component = prop_pool - (base_size_prop * prop_scaler);
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
		max_chain_single_taps = chain.get_largest_chain_single_taps();
		max_chain_ohj = chain.get_largest_chain_ohj();
		max_chain_swaps = chain.chain_swaps > chain.max_chain_swaps
							? chain.chain_swaps
							: chain.max_chain_swaps;
		max_chain_len =
		  chain._len > chain._max_len ? chain._len : chain._max_len;

		// nothing here
		if (itvhi.get_taps_nowi() == 0) {
			pmod = neutral;
			return;
		}

		// an interval with only jacks of 11112222 has no completed seq
		// the clamp should catch that scenario
		// otherwise assume conditions which continue chains are in chains
		auto taps_in_any_sequence =
		  std::clamp(mitvhi._base_types[base_single_single] +
					   mitvhi._base_types[base_single_jump] +
					   mitvhi._base_types[base_jump_single],
					 0,
					 max_chain_single_taps + max_chain_ohj);
	

		// for js we lean into max sequences more, since they're better
		// indicators of inflated difficulty

		auto ctF = static_cast<float>(max_chain_single_taps);
		auto cjF = static_cast<float>(max_chain_ohj);
		auto csF = static_cast<float>(max_chain_swaps);

		// proportion of swaps to chain size
		// any swap also requires a chain of size 2
		// some intervals have no chains, so could divide by 0
		auto chain_swap_prop = std::max(csF, 1.F) / std::max((ctF + cjF), 1.F);
		
		base_seq_prop = (ctF + cjF) / mitvhi._itvhi.get_taps_nowf();
		base_seq_prop *= fastsqrt(
		  1.F + std::max(chain_swap_pool - (chain_swap_prop * chain_swap_scaler),
					   0.F));
		set_max_seq_comp();
		max_seq_component = std::clamp(max_seq_component, 0.1F, max_mod);

		base_size_prop = taps_in_any_sequence / itvhi.get_taps_nowf();
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
		chain._max_len = 0;
		chain.max_chain_ohj = 0;
		chain.max_chain_single_taps = 0;
		max_chain_ohj = 0;
		max_chain_single_taps = 0;
		max_chain_len = 0;
		max_chain_swaps = 0;
	}
};
