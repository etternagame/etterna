#pragma once
#include "../../PatternModHelpers.h"

/// Hand-Agnostic PatternMod detecting Chordjacks.
/// Looks for continuous chords which form jacks
struct CJMod
{
	const CalcPatternMod _pmod = CJ;
	// const std::vector<CalcPatternMod> _dbg = { CJS, CJJ };
	const std::string name = "CJMod";

#pragma region params

	float min_mod = 0.6F;
	float max_mod = 1.F;
	float mod_base = 0.4F;
	float prop_buffer = 1.F;

	float total_prop_min = min_mod;
	float total_prop_max = max_mod;
	float total_prop_scaler = 5.428F;

	float jack_base = 2.F;
	float jack_min = 0.625F;
	float jack_max = 1.F;
	float jack_scaler = 1.F;

	float not_jack_pool = 1.2F;
	float not_jack_min = 0.4F;
	float not_jack_max = 1.F;
	float not_jack_scaler = 1.F;

	float vibro_flag = 1.F;
	float decay_factor = 0.1F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "mod_base", &mod_base },
		{ "prop_buffer", &prop_buffer },

		{ "total_prop_min", &total_prop_min },
		{ "total_prop_max", &total_prop_max },
		{ "total_prop_scaler", &total_prop_scaler },

		{ "jack_base", &jack_base },
		{ "jack_min", &jack_min },
		{ "jack_max", &jack_max },
		{ "jack_scaler", &jack_scaler },

		{ "not_jack_pool", &not_jack_pool },
		{ "not_jack_min", &not_jack_min },
		{ "not_jack_max", &not_jack_max },
		{ "not_jack_scaler", &not_jack_scaler },

		{ "vibro_flag", &vibro_flag },
		{ "decay_factor", &decay_factor },
	};
#pragma endregion params and param map

	float total_prop = 0.F;
	float jack_prop = 0.F;
	float not_jack_prop = 0.F;
	float pmod = min_mod;
	float t_taps = 0.F;
	float last_mod = 0.F;

	void full_reset()
	{
		last_mod = min_mod;
	}

	void decay_mod()
	{
		pmod = std::clamp(last_mod - decay_factor, min_mod, max_mod);
		last_mod = pmod;
	}

	// inline void set_dbg(std::vector<float> doot[], const int& i)
	//{
	//	doot[CJS][i] = not_jack_prop;
	//	doot[CJJ][i] = jack_prop;
	//}

	auto operator()(const metaItvInfo& mitvi) -> float
	{
		const auto& itvi = mitvi._itvi;

		if (itvi.total_taps == 0) {
			return neutral;
		}

		// no chords
		if (itvi.chord_taps == 0) {
			decay_mod();
			return pmod;
		}

		t_taps = static_cast<float>(itvi.total_taps);

		// we have at least 1 chord we want to give a little leeway for single
		// taps but not too much or sections of [12]4[123] [123]4[23] will be
		// flagged as chordjack when they're really just broken chordstream, and
		// we also want to give enough leeway so that hyperdense chordjacks at
		// lower bpms aren't automatically rated higher than more sparse jacks
		// at higher bpms
		total_prop = static_cast<float>(static_cast<float>(itvi.chord_taps) +
										prop_buffer) /
					 (t_taps - prop_buffer) * total_prop_scaler;
		total_prop =
		  std::clamp(fastsqrt(total_prop), total_prop_min, total_prop_max);

		// make sure there's at least a couple of jacks
		jack_prop =
		  std::clamp(static_cast<float>(mitvi.actual_jacks_cj) - jack_base,
					 jack_min,
					 jack_max);

		// explicitly detect broken chordstream type stuff so we can give more
		// leeway to single note jacks brop_two_return_of_brop_electric_bropaloo
		not_jack_prop = std::clamp(
		  not_jack_pool -
			(static_cast<float>(static_cast<float>(mitvi.definitely_not_jacks) *
								not_jack_scaler) /
			 t_taps),
		  not_jack_min,
		  not_jack_max);

		pmod =
		  std::clamp(total_prop * jack_prop * not_jack_prop, min_mod, max_mod);

		// ITS JUST VIBRO THEN(unique note permutations per interval < 3 ), use
		// this other places ?
		if (mitvi.basically_vibro) {
			if (mitvi.num_var == 1) {
				pmod *= 0.5F * vibro_flag;
			} else if (mitvi.num_var == 2) {
				pmod *= 0.9F * vibro_flag;
			} else if (mitvi.num_var == 3) {
				pmod *= 0.95F * vibro_flag;
			}
		}

		// set for decay
		last_mod = pmod;

		return pmod;
	}
};
