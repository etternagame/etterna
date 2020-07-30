#pragma once
#include "SequencingHelpers.h"
#include <array>

/* MS difficulty bases are going to be sequence constructed row by row here, the
 * nps base may be moved here later but not right now. we'll use statically
 * allocated arrays to built difficulty for each interval, and the output will
 * be placed into statically allocated arrays for base difficulties. (in
 * threaded calc object now, though) */

struct nps
{
	static void actual_cancer(Calc& calc, const int& hi)
	{
		for (auto itv = 0; itv < calc.numitv; ++itv) {

			auto notes = 0;

			for (auto row = 0; row < calc.itv_size.at(itv); ++row) {
				const auto& cur = calc.adj_ni.at(itv).at(row);
				notes += cur.hand_counts.at(hi);
			}

			// nps for this interval
			calc.soap.at(hi).at(NPSBase).at(itv) =
			  static_cast<float>(notes) * finalscaler * 1.6F;

			// set points for this interval
			calc.itv_points.at(hi).at(itv) = notes * 2;
		}
	}
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
		// we definitely don't want to average here because we don't want tech
		// to only be files with strong runningman pattern detection, but we
		// could probably do something more robust at some point
		return std::max(weighted_average(get_tc_base(calc), nps_base, 4.F, 9.F),
						rm_itv_max_diff);
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

	void interval_end() { _tc.interval_end(); }

	void full_reset() { interval_end(); }
};
