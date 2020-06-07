#pragma once
#include <string>
#include <array>
#include <vector>

#include "Etterna/Globals/MinaCalc/PatternModHelpers.h"
#include "Etterna/Globals/MinaCalc/Dependent/HD_Sequencers/RMSequencing.h"

using std::pair;
using std::vector;

// moving these here because this is the only mod that uses them because i'm not
// sure that they aren't horribly broken, and i don't want to spend time
// figuring it out right now
template<typename T>
inline auto
pmod_prop(T a,
		  T b,
		  const float& s,
		  const float& min,
		  const float& max,
		  const float& base = 0.F) -> float
{
	return CalcClamp(
	  (static_cast<float>(a) / static_cast<float>(b) * s) + base, min, max);
}

// template thingy for generating basic proportion scalers for pattern mods
// potentially super broken
template<typename T>
inline auto
pmod_prop(const float& pool,
		  T a,
		  T b,
		  const float& s,
		  const float& min,
		  const float& max) -> float
{
	return CalcClamp(
	  pool - (static_cast<float>(a) / static_cast<float>(b) * s), min, max);
}

struct RunningManMod
{
	const std::array<CalcPatternMod, 11> _dbg{
		RanLen,		RanAnchLen, RanAnchLenMod, RanJack,		RanOHT,		RanOffS,
		RanPropAll, RanPropOff, RanPropOHT,	RanPropOffS, RanPropJack
	};
	const CalcPatternMod _pmod = RanMan;
	const std::string name = "RunningManMod";

#pragma region params

	float min_mod = 0.95F;
	float max_mod = 1.35F;
	float base = 0.8F;
	float min_anchor_len = 5.F;
	float min_taps_in_rm = 1.F;
	float min_off_taps_same = 1.F;

	float total_prop_scaler = 1.F;
	float total_prop_min = 0.F;
	float total_prop_max = 1.F;

	float off_tap_prop_scaler = 1.3F;
	float off_tap_prop_min = 0.F;
	float off_tap_prop_max = 1.25F;
	float off_tap_prop_base = 0.05F;

	float off_tap_same_prop_scaler = 1.F;
	float off_tap_same_prop_min = 0.F;
	float off_tap_same_prop_max = 1.25F;
	float off_tap_same_prop_base = 0.25F;

	float anchor_len_divisor = 2.5F;

	float min_jack_taps_for_bonus = 1.F;
	float jack_bonus_base = 0.1F;

	float min_oht_taps_for_bonus = 1.F;
	float oht_bonus_base = 0.1F;

	// params for rm_sequencing, these define conditions for resetting
	// runningmen sequences
	float max_oht_len = 2.F;
	float max_off_spacing = 2.F;
	float max_burst_len = 6.F;
	float max_jack_len = 1.F;

	const vector<pair<std::string, float*>> _params{

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "min_anchor_len", &min_anchor_len },
		{ "min_taps_in_rm", &min_taps_in_rm },
		{ "min_off_taps_same", &min_off_taps_same },

		{ "total_prop_scaler", &total_prop_scaler },
		{ "total_prop_min", &total_prop_min },
		{ "total_prop_max", &total_prop_max },

		{ "off_tap_prop_scaler", &off_tap_prop_scaler },
		{ "off_tap_prop_min", &off_tap_prop_min },
		{ "off_tap_prop_max", &off_tap_prop_max },
		{ "off_tap_prop_base", &off_tap_prop_base },

		{ "off_tap_same_prop_scaler", &off_tap_same_prop_scaler },
		{ "off_tap_same_prop_min", &off_tap_same_prop_min },
		{ "off_tap_same_prop_max", &off_tap_same_prop_max },
		{ "off_tap_same_prop_base", &off_tap_same_prop_base },

		{ "anchor_len_divisor", &anchor_len_divisor },

		{ "min_jack_taps_for_bonus", &min_jack_taps_for_bonus },
		{ "jack_bonus_base", &jack_bonus_base },

		{ "min_oht_taps_for_bonus", &min_oht_taps_for_bonus },
		{ "oht_bonus_base", &oht_bonus_base },

		// params for rm_sequencing
		{ "max_oht_len", &max_oht_len },
		{ "max_off_spacing", &max_off_spacing },
		{ "max_burst_len", &max_burst_len },
		{ "max_jack_len", &max_jack_len },
	};
#pragma endregion params and param map

	bool debug_lmao = false;

	// stuff for making mod
	RM_Sequencer rms[2];
	// longest sequence for this interval
	RM_Sequencer rm;

	int test = 0;
	float total_prop = 0.F;
	float off_tap_prop = 0.F;
	float off_tap_same_prop = 0.F;

	float anchor_len_comp = 0.F;
	float jack_bonus = 0.F;
	float oht_bonus = 0.F;

	float pmod = neutral;

	inline void full_reset()
	{
		rm.full_reset();
		for (auto& rm : rms) {
			rm.full_reset();
		}

		test = 0;
		total_prop = 0.F;
		off_tap_prop = 0.F;
		off_tap_same_prop = 0.F;

		anchor_len_comp = 0.F;
		jack_bonus = 0.F;
		oht_bonus = 0.F;

		pmod = neutral;
	}

	inline void setup(vector<float> doot[], const int& size)
	{
		// don't try to figure out which column a prospective anchor is on, just
		// run two passes with each assuming a different column
		rms[0].anchor_col = col_left;
		rms[1].anchor_col = col_right;
		rms[0].set_params(
		  max_oht_len, max_off_spacing, max_burst_len, max_jack_len);
		rms[1].set_params(
		  max_oht_len, max_off_spacing, max_burst_len, max_jack_len);

		doot[_pmod].resize(size);
		if (debug_lmao) {
			for (auto& mod : _dbg) {
				doot[mod].resize(size);
			}
		}
	}

	inline void advance_sequencing(const col_type& ct,
								   const base_pattern_type& bt,
								   const meta_type& mt,
								   const float& row_time,
								   const int& offhand_taps)
	{
		// sequencing objects should be moved int mhi itself
		for (auto& rm : rms) {
			rm(ct, bt, mt, row_time, offhand_taps);
		}

		// use the biggest anchor that has existed in this interval
		test = rms[0].anchor_len > rms[1].anchor_len ? 0 : 1;

		if (rms[test].anchor_len > rm.anchor_len) {
			rm = rms[test];
		}
	}

	inline void set_dbg(vector<float> doot[], const int& i)
	{
		if (debug_lmao) {
			doot[RanLen][i] = 1.F;
			doot[RanAnchLen][i] =
			  (static_cast<float>(rm.anchor_len) / 30.F) + 0.5F;
			doot[RanAnchLenMod][i] = anchor_len_comp;
			doot[RanOHT][i] = static_cast<float>(rm.oht_taps);
			doot[RanOffS][i] = static_cast<float>(rm.off_taps_same);
			doot[RanJack][i] = static_cast<float>(rm.jack_taps);
			doot[RanPropAll][i] = total_prop;
			doot[RanPropOff][i] = off_tap_prop;
			doot[RanPropOffS][i] = off_tap_same_prop;
			doot[RanPropOHT][i] = oht_bonus;
			doot[RanPropJack][i] = jack_bonus;
		}
	}

	inline auto operator()(vector<float> doot[], const int& i) -> float
	{
		// we could mni check for empty intervals like the other mods but it
		// doesn't really matter and this is probably more useful for debug
		// output

		// we could decay in this but it may conflict/be redundant with how
		// runningmen sequences are constructed, if decays are used we would
		// probably generate the mod not from the highest of any interval, but
		// from whatever sequences are still alive by the end

		// min mod optimization
		if (rm.anchor_len < min_anchor_len || rm.ran_taps < min_taps_in_rm ||
			rm.off_taps_same < min_off_taps_same) {

			set_dbg(doot, i);
			rm.reset();

			return min_mod;
		}

		// the pmod template stuff completely broke the js/hs/cj mods.. so..
		// these might also be broken... investigate later

		// taps in runningman / total taps in interval... i think? can't
		// remember when i reset total taps tbh.. this might be useless
		total_prop = 1.F;

		// number anchor taps / number of non anchor taps
		off_tap_prop = fastpow(pmod_prop(rm.anchor_len,
										 rm.ran_taps,
										 off_tap_prop_scaler,
										 off_tap_prop_min,
										 off_tap_prop_max,
										 off_tap_prop_base),
							   2.F);

		// number of same hand off anchor taps / anchor taps, basically stuff is
		// really hard when this is high (a value of 0.5 is a triplet every
		// other anchor)
		off_tap_same_prop = pmod_prop(rm.off_taps_same,
									  rm.anchor_len,
									  off_tap_same_prop_scaler,
									  off_tap_same_prop_min,
									  off_tap_same_prop_max,
									  off_tap_same_prop_base);

		// anchor length component
		anchor_len_comp =
		  static_cast<float>(rm.anchor_len) / anchor_len_divisor;

		// jacks in anchor component, give a small bonus i guess
		jack_bonus =
		  rm.jack_taps >= min_jack_taps_for_bonus ? jack_bonus_base : 0.F;

		// ohts in anchor component, give a small bonus i guess
		// not done
		oht_bonus =
		  rm.oht_taps >= min_oht_taps_for_bonus ? oht_bonus_base : 0.F;

		// we could scale the anchor to speed if we want but meh
		// that's really complicated/messy/error prone
		pmod = base + anchor_len_comp + jack_bonus + oht_bonus;
		pmod = CalcClamp(
		  fastsqrt(pmod * total_prop * off_tap_prop /** off_tap_same_prop*/),
		  min_mod,
		  max_mod);

		doot[_pmod][i] = pmod;
		set_dbg(doot, i);

		// reset interval highest when we're done
		rm.reset();
	}
};
