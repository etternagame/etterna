#pragma once
#include <string>
#include <array>
#include <vector>

#include "Etterna/Globals/MinaCalc/PatternModHelpers.h"
#include "Etterna/Globals/MinaCalc/Dependent/HD_Sequencers/RMSequencing.h"

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
	/*const std::array<CalcPatternMod, 11> _dbg{
		RanLen,		RanAnchLen, RanAnchLenMod, RanJack,		RanOHT,		RanOffS,
		RanPropAll, RanPropOff, RanPropOHT,	   RanPropOffS, RanPropJack
	};*/
	const CalcPatternMod _pmod = RanMan;
	const std::string name = "RunningManMod";

#pragma region params

	float min_mod = 1.F;
	float max_mod = 1.08F;
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
	float max_off_len = 3.F;
	float max_ot_sh_len = 2.F;
	float max_burst_len = 6.F;
	float max_jack_len = 3.F;
	float max_anch_len = 3.F;

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
		{ "max_off_len", &max_off_len },
		{ "max_off_len", &max_ot_sh_len },
		{ "max_burst_len", &max_burst_len },
		{ "max_jack_len", &max_jack_len },
		{ "max_anch_len", &max_anch_len },
	};
#pragma endregion params and param map

	// stuff for making mod
	std::array<RM_Sequencer, num_cols_per_hand> rms;

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

	inline void setup()
	{
		// don't try to figure out which column a prospective anchor is on, just
		// run two passes with each assuming a different column

		for (auto& c : ct_loop_no_jumps) {
			rms[c]._ct = c;
			rms[c].set_params(max_oht_len,
							   max_off_len,
							   max_ot_sh_len,
							   max_burst_len,
							   max_jack_len,
							   max_anch_len);
		}
	}

	inline void advance_off_hand_sequencing()
	{
		for (auto& c : ct_loop_no_jumps) {
			rms.at(c).advance_off_hand_sequencing();
		}
	}

	inline void advance_sequencing(const col_type& ct,
								   const base_type& bt,
								   const meta_type& mt,
								   const AnchorSequencer& as)
	{
		for (auto& c : ct_loop_no_jumps) {
			rms.at(c)(ct, bt, mt, as.anch[c]);
		}
	}

	[[nodiscard]] inline auto get_highest_anchor_difficulty() const -> float
	{
		return max(rms.at(col_left).get_difficulty(),
				   rms.at(col_right).get_difficulty());
	}

	[[nodiscard]] inline auto get_rm_with_higher_difficulty() const
	  -> const RunningMan&
	{
		return rms.at(col_left).get_difficulty() >
				   rms.at(col_right).get_difficulty()
				 ? rms.at(col_left)._rm
				 : rms.at(col_right)._rm;
	}

	inline auto operator()() -> float
	{
		const auto& rm = get_rm_with_higher_difficulty();
		// we could mni check for empty intervals like the other mods but it
		// doesn't really matter and this is probably more useful for debug
		// output

		// we could decay in this but it may conflict/be redundant with how
		// runningmen sequences are constructed, if decays are used we would
		// probably generate the mod not from the highest of any interval, but
		// from whatever sequences are still alive by the end

		// min mod optimization
		if (rm._len < min_anchor_len || rm.ran_taps < min_taps_in_rm ||
			rm.off_taps_sh < min_off_taps_same) {

			return min_mod;
		}

		// the pmod template stuff completely broke the js/hs/cj mods.. so..
		// these might also be broken... investigate later

		// taps in runningman / total taps in interval... i think? can't
		// remember when i reset total taps tbh.. this might be useless
		total_prop = 1.F;

		// number anchor taps / number of non anchor taps
		off_tap_prop = fastpow(pmod_prop(rm._len,
										 rm.ran_taps,
										 off_tap_prop_scaler,
										 off_tap_prop_min,
										 off_tap_prop_max,
										 off_tap_prop_base),
							   2.F);

		// number of same hand off anchor taps / anchor taps, basically stuffs
		// is really hard when this is high (a value of 0.5 is a triplet every
		// other anchor)
		off_tap_same_prop = pmod_prop(rm.off_taps_sh,
									  rm._len,
									  off_tap_same_prop_scaler,
									  off_tap_same_prop_min,
									  off_tap_same_prop_max,
									  off_tap_same_prop_base);

		// anchor length component
		anchor_len_comp = static_cast<float>(rm._len) / anchor_len_divisor;

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

		return pmod;
	}
};
