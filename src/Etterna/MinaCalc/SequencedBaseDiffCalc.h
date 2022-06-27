#pragma once
#include "SequencingHelpers.h"
#include <array>

/* MS difficulty bases are going to be sequence constructed row by row here, the
 * nps base may be moved here later but not right now. we'll use statically
 * allocated arrays to built difficulty for each interval, and the output will
 * be placed into statically allocated arrays for base difficulties. (in
 * threaded calc object now, though) */

/// required percentage of average notes to pass
constexpr float min_threshold = 0.65F;
static const float downscale_logbase = std::log(6.2F);

constexpr float scaler_for_ms_base = 1.175F;
// i do not know a proper name for these
constexpr float ms_base_finger_weighter_2 = 9.F;
constexpr float ms_base_finger_weighter = 5.5F;

static auto
CalcMSEstimate(std::vector<float>& input, const int& burp) -> float
{
	// how many ms values we use from here, if there are fewer than this
	// number we'll mock up some values to water down intervals with a
	// single extremely fast minijack, if there are more, we will truncate
	unsigned int num_used = burp;

	if (input.empty()) {
		return 0.F;
	}

	// avoiding this for now because of smoothing
	// single ms value, dunno if we want to do this? technically the tail
	// end of an insanely hard burst that gets lopped off at the last note
	// is still hard? if (input.size() < 2) return 1.f;

	// sort before truncating/filling
	std::sort(input.begin(), input.end());

	// truncate if we have more values than what we care to sample, we're
	// looking for a good estimate of the hardest part of this interval
	// if above 1 and below used_ms_vals, fill up the stuff with dummies
	// my god i was literally an idiot for doing what i was doing before
	static const float ms_dummy = 360.F;

	// mostly try to push down stuff like jumpjacks, not necessarily to push
	// up "complex" stuff (this will push up intervals with few fast ms
	// values kinda hard but it shouldn't matter as their base ms diff
	// should be extremely low
	float cv_yo = cv_trunc_fill(input, burp, ms_dummy) + 0.5F;
	cv_yo = std::clamp(cv_yo, 0.5F, 1.25F);

	// basically doing a jank average, bigger m = lower difficulty
	float m = sum_trunc_fill(input, burp, ms_dummy);

	// add 1 to num_used because some meme about sampling
	// same thing as jack stuff, convert to bpm and then nps
	float bpm_est = ms_to_bpm(m / (num_used + 1));
	float nps_est = bpm_est / 15.F;
	float fdiff = nps_est * cv_yo;
	return fdiff;
}

struct nps
{
	/// determine NPSBase, itv_points, CJBase for this hand
	static void actual_cancer(Calc& calc, const int& hand)
	{
		// finger row times
		auto last_left_row_time = s_init;
		auto last_right_row_time = s_init;

		auto scaly_ms_estimate = [](std::vector<float>& input,
								const float& scaler) {
			float o = CalcMSEstimate(input, 3);
			if (input.size() > 3) {
				o = std::max(o, CalcMSEstimate(input, 4) * scaler);
			}
			if (input.size() > 4) {
				o = std::max(o, CalcMSEstimate(input, 5) * scaler * scaler);
			}
			return o;
		};

		for (auto itv = 0; itv < calc.numitv; ++itv) {
			auto notes = 0;

			// times between rows for this interval for each finger
			std::vector<float> left_finger_ms{};
			std::vector<float> right_finger_ms{};

			for (auto row = 0; row < calc.itv_size.at(itv); ++row) {
				const auto& cur = calc.adj_ni.at(itv).at(row);
				notes += cur.hand_counts.at(hand);

				const auto& crt = cur.row_time;
				switch (determine_col_type(cur.row_notes, hand_col_ids[hand])) {
					case col_left:
						if (last_left_row_time != s_init) {
							const auto left_ms =
							  ms_from(crt, last_left_row_time);
							left_finger_ms.push_back(left_ms);
						}
						last_left_row_time = crt;
						break;
					case col_right:
						if (last_right_row_time != s_init) {
							const auto right_ms =
							  ms_from(crt, last_right_row_time);
							right_finger_ms.push_back(right_ms);
						}
						last_right_row_time = crt;
						break;
					case col_ohjump:
						if (last_right_row_time != s_init) {
							const auto right_ms =
							  ms_from(crt, last_right_row_time);
							right_finger_ms.push_back(right_ms);
						}
						if (last_left_row_time != s_init) {
							const auto left_ms =
							  ms_from(crt, last_left_row_time);
							left_finger_ms.push_back(left_ms);
						}
						last_left_row_time = crt;
						last_right_row_time = crt;
						break;
					case col_empty:
					case col_init:
					default:
						// none
						break;
				}
			}

			// nps for this interval
			const auto nps = static_cast<float>(notes) * finalscaler * 1.6F;
			calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv) = nps;

			// ms base for this interval
			const auto left =
			  scaly_ms_estimate(left_finger_ms, scaler_for_ms_base);
			const auto right =
			  scaly_ms_estimate(right_finger_ms, scaler_for_ms_base);
			float msdiff = 0.F;
			if (left > right) {
				msdiff = weighted_average(left,
										  right,
										  ms_base_finger_weighter,
										  ms_base_finger_weighter_2);
			} else {
				msdiff = weighted_average(right,
										  left,
										  ms_base_finger_weighter,
										  ms_base_finger_weighter_2);
			}
			const auto msbase = finalscaler * msdiff;
			calc.init_base_diff_vals.at(hand).at(MSBase).at(itv) = msbase;
			/////

			// set points for this interval
			calc.itv_points.at(hand).at(itv) = notes * 2;
		}
	}

	/// determine grindscaler using smoothed npsbase
	static void grindscale(Calc& calc) {
		auto populated_intervals = 0;
		auto avg_notes = 0.F;
		for (auto itv = 0; itv < calc.numitv; ++itv) {
			auto notes = 0.F;

			for (auto& hand : both_hands) {
				notes += calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv);
			}

			if (notes > 0) {
				avg_notes += notes;
				populated_intervals++;
			}
		}

		if (populated_intervals > 0) {
			const auto empty_intervals =
			  static_cast<float>(calc.numitv - populated_intervals);
			avg_notes /= populated_intervals;

			auto failed_intervals = 0;

			for (auto itv = 0; itv < calc.numitv; itv++) {
				auto notes = 0.F;
				for (auto& hand : both_hands)
					notes +=
					  calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv);

				// count only intervals with notes
				if (notes > 0.F && notes < avg_notes * min_threshold)
					failed_intervals++;
			}

			// base grindscaler on how many intervals are passing
			// if the entire file is just single taps or empty:
			//   ask yourself... is it really worth playing?
			const auto file_length =
			  itv_idx_to_time(populated_intervals - failed_intervals);
			// log minimum but if you move this you need to move the log base
			const auto ping = .3F;
			const auto timescaler =
			  (ping * (std::log(file_length + 1) / downscale_logbase)) + ping;

			calc.grindscaler = std::clamp(timescaler, .1F, 1.F);
		} else {
			calc.grindscaler = .1F;
		}
	}
};

struct ceejay
{
	void update_flags(const unsigned& row_notes, const int& row_count)
	{
		is_cj = last_row_count > 1 && row_count > 1;
		was_cj = last_row_count > 1 && last_last_row_count > 1;

		is_scj = (row_count == 1 && last_row_count > 1) &&
				 ((row_notes & last_row_notes) != 0u);

		is_at_least_3_note_anch =
		  ((row_notes & last_row_notes) & last_last_row_notes) != 0u;

		last_last_row_count = last_row_count;
		last_row_count = row_count;

		last_last_row_notes = last_row_notes;
		last_row_notes = row_notes;

		last_was_3_note_anch = is_at_least_3_note_anch;
	}

	void advance_base(const float& any_ms, Calc& calc)
	{
		if (row_counter >= max_rows_for_single_interval) {
			{
				{
					return;
				}
			}
		}

		// pushing back ms values, so multiply to nerf
		float pewpew = 3.F;

		if (is_at_least_3_note_anch && last_was_3_note_anch) {
			// biggy boy anchors and beyond
			pewpew = 1.F;
		} else if (is_at_least_3_note_anch) {
			// big boy anchors
			pewpew = 1.F;
		} else {
			// single note
			if (!is_cj) {
				if (is_scj) {
					// was cj a little bit ago..
					if (was_cj) {
						// single note jack with 2 chords behind it
						pewpew = 1.25F;
					} else {
						// single note, not a jack, 2 chords behind
						// it
						pewpew = 1.5F;
					}
				}
			} else {
				// actual cj
				if (was_cj) {
					// cj now and was cj before, but not necessarily
					// with strong anchors
					pewpew = 1.15F;
				} else {
					// cj now but wasn't even cj before
					pewpew = 1.25F;
				}
			}
		}

		// single note streams / regular jacks should retain the 3x
		// multiplier

		calc.cj_static.at(row_counter) = std::max(75.F, any_ms * pewpew);
		++row_counter;
	}

	// final output difficulty for this interval
	auto get_itv_diff(Calc& calc) const -> float
	{
		if (row_counter == 0) {
			return 0.F;
		}

		float ms_total = 0.F;
		for (int i = 0; i < row_counter; ++i) {
			{
				{
					ms_total += calc.cj_static.at(i);
				}
			}
		}

		float ms_mean = ms_total / static_cast<float>(row_counter);
		return ms_to_scaled_nps(ms_mean);
	}

	void interval_end() { row_counter = 0; }
	void full_reset()
	{
		is_cj = false;
		was_cj = false;
		is_scj = false;
		is_at_least_3_note_anch = false;
		last_was_3_note_anch = false;

		last_row_count = 0;
		last_last_row_count = 0;

		last_row_notes = 0U;
		last_last_row_notes = 0U;
	}

  private:
	int row_counter = 0;

	bool is_cj = false;
	bool was_cj = false;
	bool is_scj = false;
	bool is_at_least_3_note_anch = false;
	bool last_was_3_note_anch = false;

	int last_row_count = 0;
	int last_last_row_count = 0;

	unsigned last_row_notes = 0U;
	unsigned last_last_row_notes = 0U;
};

struct techyo
{
	// if this looks ridiculous, that's because it is
	void advance_base(const SequencerGeneral& seq,
					  const col_type& ct,
					  Calc& calc)
	{
		if (row_counter >= max_rows_for_single_interval) {
			return;
		}

		const auto a = seq.get_sc_ms_now(ct);
		float b;
		if (ct == col_ohjump) {
			b = seq.get_sc_ms_now(ct, false);
		} else {
			b = seq.get_cc_ms_now();
		}

		const auto c = fastsqrt(a) * fastsqrt(b);

		auto pineapple = seq._mw_any_ms.get_cv_of_window(4);
		auto porcupine = seq._mw_sc_ms[col_left].get_cv_of_window(4);
		auto sequins = seq._mw_sc_ms[col_right].get_cv_of_window(4);
		const auto oioi = 0.5F;
		pineapple = std::clamp(pineapple + oioi, oioi, 1.F + oioi);
		porcupine = std::clamp(porcupine + oioi, oioi, 1.F + oioi);
		sequins = std::clamp(sequins + oioi, oioi, 1.F + oioi);

		const auto scoliosis = seq._mw_sc_ms[col_left].get_now();
		const auto poliosis = seq._mw_sc_ms[col_right].get_now();
		float obliosis;

		if (ct == col_left) {
			obliosis = poliosis / scoliosis;
		} else {
			obliosis = scoliosis / poliosis;
		}

		obliosis = std::clamp(obliosis, 1.F, 10.F);
		auto pewp = fastsqrt(div_high_by_low(scoliosis, poliosis) - 1.F);

		pewp /= obliosis;
		const auto vertebrae = std::clamp(
		  ((pineapple + porcupine + sequins) / 3.F) + pewp, oioi, 1.F + oioi);

		teehee(c / vertebrae);

		calc.tc_static.at(row_counter) = teehee.get_mean_of_window(2);
		++row_counter;
	}

	void advance_rm_comp(const float& rm_diff)
	{
		rm_itv_max_diff = std::max(rm_itv_max_diff, rm_diff);
	}

	// for debug
	[[nodiscard]] auto get_itv_rma_diff() const -> float
	{
		return rm_itv_max_diff;
	}

	// final output difficulty for this interval, merges base diff, runningman
	// anchor diff
	[[nodiscard]] auto get_itv_diff(const float& nps_base, Calc& calc) const
	  -> float
	{
		// for now do simple thing, for this interval either use the higher
		// between weighted adjusted ms/nps base and runningman diff
		// we definitely don't want to pure average here because we don't want tech
		// to only be files with strong runningman pattern detection, but we
		// could probably do something more robust at some point
		auto tc = weighted_average(get_tc_base(calc), nps_base, 4.F, 9.F);
		auto rm = rm_itv_max_diff;
		if (rm >= tc) {
			// for rm dominant intervals, use tc to drag diff down
			// weight should be [0,1]
			// 1 -> all rm
			// 0 -> all tc
			constexpr float weight = 0.9F;
			rm = weighted_average(rm, tc, weight, 1.F);
		}
		return std::max(tc, rm);
	}

	void interval_end()
	{
		row_counter = 0;
		rm_itv_max_diff = 0.F;
	}

  private:
	// how many non-empty rows have we seen
	int row_counter = 0;

	// jank stuff.. keep a small moving average of the base diff
	CalcMovingWindow<float> teehee;

	// max value of rm diff for this interval, this will be an exception to the
	// only storing ms rule, rm diff will be stored as a pre-converted diff
	// value because rm_sequencing may adjust the scaled diff using the rm
	// components in ways we can't emulate here (nor should we try)
	float rm_itv_max_diff = 0.F;

	// get the interval base diff, which will then be merged via weighted
	// average with npsbase, and then compared to max_rm diff
	[[nodiscard]] auto get_tc_base(Calc& calc) const -> float
	{
		if (row_counter == 0) {
			return 0.F;
		}

		auto ms_total = 0.F;
		for (auto i = 0; i < row_counter; ++i) {
			ms_total += calc.tc_static.at(i);
		}

		const auto ms_mean = ms_total / static_cast<float>(row_counter);
		return ms_to_scaled_nps(ms_mean);
	}
};

struct diffz
{
	nps _nps;
	techyo _tc;
	ceejay _cj;

	void interval_end()
	{
		_tc.interval_end();
		_cj.interval_end();
	}

	void full_reset()
	{
		interval_end();
		_cj.full_reset();
	}
};
