#pragma once
#include <string>
#include <array>
#include <vector>

#include "Etterna/Models/NoteData/NoteDataStructures.h"

using std::vector;
using std::pair;

// since the calc skillset balance now operates on +- rather than
// just - and then normalization, we will use this to depress the
// stream rating for non-stream files.

static const CalcPatternMod _pmod = Stream;
static const std::string name = "StreamMod";
static const int _tap_size = single;

struct StreamMod
{
	inline const CalcPatternMod& get_mod() { return _pmod; }
	inline const std::string& get_name() { return name; }

#pragma region params
	float min_mod = 0.6F;
	float max_mod = 1.0F;
	float prop_buffer = 1.F;
	float prop_scaler = 1.428F; // ~10/7

	float jack_pool = 4.F;
	float jack_comp_min = 0.5F;
	float jack_comp_max = 1.F;

	const vector<pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "prop_buffer", &prop_buffer },
		{ "prop_scaler", &prop_scaler },

		{ "jack_pool", &jack_pool },
		{ "jack_comp_min", &jack_comp_min },
		{ "jack_comp_max", &jack_comp_max },
	};
#pragma endregion params and param map

	float prop_component = 0.F;
	float jack_component = 0.F;
	float pmod = min_mod;

	inline auto operator()(const metaItvInfo& mitvi, vector<float> doot[]) -> float
	{
		const auto& itvi = mitvi._itvi;

		// 1 tap is by definition a single tap
		if (itvi.total_taps < 2) {
			return neutral;
		}

		if (itvi.taps_by_size[_tap_size] == 0) {
			return min_mod;
		}

		// we want very light js to register as stream,
		// something like jumps on every other 4th, so 17/19
		// ratio should return full points, but maybe we should
		// allow for some leeway in bad interval slicing this
		// maybe doesn't need to be so severe, on the other
		// hand, maybe it doesn'ting need to be not needing'nt
		// to be so severe

		// we could make better use of sequencing here since now it's easy

		prop_component =
		  static_cast<float>(itvi.taps_by_size[_tap_size] + prop_buffer) /
		  static_cast<float>(itvi.total_taps - prop_buffer) * prop_scaler;

		// allow for a mini/triple jack in streams.. but not more than that
		jack_component = CalcClamp(
		  jack_pool - mitvi.actual_jacks, jack_comp_min, jack_comp_max);
		pmod = fastsqrt(prop_component * jack_component);
		pmod = CalcClamp(pmod, min_mod, max_mod);

		// actual mod
		doot[_pmod][mitvi._idx] = pmod;
	}
};
