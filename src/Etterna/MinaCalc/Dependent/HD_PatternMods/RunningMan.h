#pragma once
#include "../../PatternModHelpers.h"
#include "../HD_Sequencers/RMSequencing.h"

/** Hand-Dependent PatternMod detecting a RunningMen.
 * Unlike other pattern mods runningman sequencing has 2 core purposes, the
 * primary one involves tracking the anchor speed of a runningman sequence and
 * using that plus other characteristics of the runningman to guess at a more ms
 * oriented baseline difficulty, which gets rolled into the tech base
 * calculations, this happens during the main row loop in ulbu, but not inside
 * the pattern mod loop. The other is the usual pattern mod generation, which
 * generates a pattern mod out purely the characteristics, independent of the
 * anchor speed. This is to slightly push up small runningman anchors inside
 * streams, or js, etc, and to fully push up much heavier runningman oriented
 * patterns in those same skillsets (policy in the sky, etc) */
struct RunningManMod
{
	const CalcPatternMod _pmod = RanMan;
	const std::string name = "RunningManMod";

#pragma region params

	float min_mod = 1.F;
	float max_mod = 1.1F;
	float base = 0.5F;
	float min_anchor_len = 5.F;
	float min_taps_in_rm = 1.F;
	float min_off_taps_same = 1.F;

	float offhand_tap_prop_scaler = 1.F;
	float offhand_tap_prop_min = 0.F;
	float offhand_tap_prop_max = 1.F;
	float offhand_tap_prop_base = 1.7F;

	float offhand_tap_prop_anch_diff_base = 1.7F;
	float offhand_tap_prop_anch_diff_scaler = 1.1F;
	float offhand_tap_prop_anch_diff_min = 0.75F;
	float offhand_tap_prop_anch_diff_max = 1.F;

	float off_tap_same_prop_scaler = 1.F;
	float off_tap_same_prop_min = 0.F;
	float off_tap_same_prop_max = 1.25F;
	float off_tap_same_prop_base = 0.8F;

	float anchor_len_divisor = 5.F;
	float anchor_len_comp_min = 0.F;
	float anchor_len_comp_max = 1.25F;

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
	float max_anch_len = 5.F;

	const std::vector<std::pair<std::string, float*>> _params{

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		{ "min_anchor_len", &min_anchor_len },
		{ "min_taps_in_rm", &min_taps_in_rm },
		{ "min_off_taps_same", &min_off_taps_same },

		{ "offhand_tap_prop_scaler", &offhand_tap_prop_scaler },
		{ "offhand_tap_prop_min", &offhand_tap_prop_min },
		{ "offhand_tap_prop_max", &offhand_tap_prop_max },
		{ "offhand_tap_prop_base", &offhand_tap_prop_base },

		{ "offhand_tap_prop_anch_diff_base", &offhand_tap_prop_anch_diff_base },
		{ "offhand_tap_prop_anch_diff_scaler", &offhand_tap_prop_anch_diff_scaler },
		{ "offhand_tap_prop_anch_diff_min", &offhand_tap_prop_anch_diff_min },
		{ "offhand_tap_prop_anch_diff_max", &offhand_tap_prop_anch_diff_max },

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
		{ "max_ot_sh_len", &max_ot_sh_len },
		{ "max_burst_len", &max_burst_len },
		{ "max_jack_len", &max_jack_len },
		{ "max_anch_len", &max_anch_len },
	};
#pragma endregion params and param map

	// stuff for making mod
	std::array<RM_Sequencer, num_cols_per_hand> rms;

	// for an interval, active rm sequence with the highest difficulty
	RM_Sequencer highest_rm;

	int test = 0;
	float offhand_tap_prop = 0.F;
	float off_tap_same_prop = 0.F;

	float anchor_len_comp = 0.F;
	float jack_bonus = 0.F;
	float oht_bonus = 0.F;

	float pmod = neutral;

	void full_reset()
	{
		for (auto& rm : rms) {
			rm.full_reset();
		}

		offhand_tap_prop = 0.F;
		off_tap_same_prop = 0.F;

		anchor_len_comp = 0.F;
		jack_bonus = 0.F;
		oht_bonus = 0.F;

		pmod = neutral;
	}

	/* keep parallel rm sequencers for both left and right column for each
	 * hand, this way we don't have to worry about trying to figure out
	 * which column the runningman anchor should be on */
	void setup()
	{
		for (const auto& c : ct_loop_no_jumps) {
			rms.at(c)._ct = c;
			rms.at(c).set_params(max_oht_len,
								 max_off_len,
								 max_ot_sh_len,
								 max_burst_len,
								 max_jack_len,
								 max_anch_len);
		}
	}

	void advance_off_hand_sequencing()
	{
		for (const auto& c : ct_loop_no_jumps) {
			rms.at(c).advance_off_hand_sequencing();
		}
	}

	void advance_sequencing(const col_type& ct,
							const base_type& bt,
							const meta_type& mt,
							const AnchorSequencer& as)
	{
		for (const auto& c : ct_loop_no_jumps) {
			rms.at(c)(ct, bt, mt, *as.anch.at(c));
		}

		highest_rm = get_active_rm_with_higher_difficulty();
	}

	[[nodiscard]] auto get_highest_anchor_difficulty() const -> float
	{

		/* see off_hand_tap_prop for a detailed explanation, basically only the
		 * rm mod was downscaling rolls, short burst rolls that escaped roll
		 * detection but flagged high on on rm diff were super overrated without
		 * this adjustment, so we should probably calculated and apply it here
		 * as well- this is called immediately after advance_sequencing, so
		 * we've already determined which sequence to use as rm */

		float oht_p = offhand_tap_prop_anch_diff_base -
					  (highest_rm._rm.get_offhand_tap_prop() *
					   offhand_tap_prop_anch_diff_scaler);

		oht_p = std::clamp(oht_p,
						   offhand_tap_prop_anch_diff_min,
						   offhand_tap_prop_anch_diff_max);

		return highest_rm.get_difficulty() * oht_p;
	}

	[[nodiscard]] auto get_active_rm_with_higher_difficulty() const
	  -> RM_Sequencer
	{
		if (rms.at(col_left)._status == rm_running &&
			rms.at(col_right)._status == rm_running) {

			return rms.at(col_left).get_difficulty() >
					   rms.at(col_right).get_difficulty()
					 ? rms.at(col_left)
					 : rms.at(col_right);
		}

		return rms.at(col_left)._status == rm_running ? rms.at(col_left)
													  : rms.at(col_right);
	}

	/* Note: this mod is only used for pushing up runningmen focused stream/js
	 * _patterns_, the anchor difficulty isn't used here, that's used in tech.
	 * since we don't have to push the mod to extreme levels anymore to get
	 * runningmen registering in tech, we can tune this mod with the expectation
	 * that it will push up some files that don't need to be pushed up */
	void set_pmod(const int& total_taps)
	{
		/* nothing here */
		if (total_taps == 0) {
			pmod = neutral;
			return;
		}

		const auto& rm = highest_rm._rm;

		/* we could decay in this but it may conflict/be redundant with how
		 * runningmen sequences are constructed, if decays are used we would
		 * probably generate the mod not from the highest of any interval, but
		 * from whatever sequences are still alive by the end */

		// min mod optimization
		if (rm._len < min_anchor_len || rm.ran_taps < min_taps_in_rm ||
			rm.off_taps_sh < min_off_taps_same) {
			pmod = min_mod;
			return;
		}

		/* the larger the share of off hand taps to anchor taps, the higher the
		 * probability we're just looking at something like rolls, assuming
		 * 1234123412341234 (which may technically count as a runningmen
		 * depending on the logic params), off hand taps will outnumber the
		 * anchor taps 2:1, whereas traditional runningmen generally have under
		 * a 1:1 ratio (1:1 but this will be split among same hand off anchor
		 * taps as well, essentially, here the lower the better (as long as we
		 * properly eliminate ohts from registering)). It seems prudent for this
		 * to only be a nerf. Note: this will also push down detection for heavy
		 * ohjump usage on the other hand, which is probably? good */
		offhand_tap_prop = offhand_tap_prop_base - (rm.get_offhand_tap_prop() *
													offhand_tap_prop_scaler);
		offhand_tap_prop = std::clamp(
		  offhand_tap_prop, offhand_tap_prop_min, offhand_tap_prop_max);

		/* number of same hand off anchor taps / anchor taps, basically stuffs
		 * is really hard when this is high (a value of 0.5 is a triplet every
		 * other anchor), although actually really anything above 0.75 is
		 * basically oht, but we should be screening out pure ohts from this
		 * anyway, actually actually rolls will technically have a 1:1 ratio
		 * between anchor taps and same hand taps as well.. but those should
		 * also be diluted via detection logic and hopefully other things, if
		 * not we could do something with 0.5 designated as an inflection point
		 * but that seems hacky */
		off_tap_same_prop =
		  off_tap_same_prop_base +
		  (rm.get_off_tap_same_prop() * off_tap_same_prop_scaler);

		off_tap_same_prop = std::clamp(
		  off_tap_same_prop, off_tap_same_prop_min, off_tap_same_prop_max);

		/* anchor length component, we want longer runningmen to inherently
		 * register more strongly, but not to an infinite degree, we don't want
		 * long sequences of slow runningmen to ramp up the mod to max value
		 * just through length alone */
		anchor_len_comp = static_cast<float>(rm._len) / anchor_len_divisor;
		anchor_len_comp =
		  std::clamp(anchor_len_comp, anchor_len_comp_min, anchor_len_comp_max);

		// jacks in anchor component, give a small bonus i guess
		jack_bonus =
		  rm.jack_taps >= min_jack_taps_for_bonus ? jack_bonus_base : 0.F;

		// ohts in anchor component, give a small bonus i guess
		oht_bonus =
		  rm.oht_taps >= min_oht_taps_for_bonus ? oht_bonus_base : 0.F;

		pmod = base + anchor_len_comp + jack_bonus + oht_bonus;
		pmod = std::clamp(fastsqrt(pmod * off_tap_same_prop * offhand_tap_prop),
						  min_mod,
						  max_mod);
	}

	[[nodiscard]] auto operator()(const int& total_taps) -> float
	{
		set_pmod(total_taps);

		interval_end();
		return pmod;
	}

	void interval_end() { highest_rm.full_reset(); }
};
