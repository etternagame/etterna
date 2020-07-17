#pragma once
#include <vector>
#include <algorithm>

/* enums and other definitions that only the calcmain and its components need */

static const float max_rating = 100.F;
static const float min_rating = 0.F;
static const float default_score_goal = 0.93F;
static const float low_acc_cutoff = 0.9F;
static const float ssr_goal_cap = 0.965F;

static const std::vector<float> dimples_the_all_zero_output{
	min_rating, min_rating, min_rating, min_rating,
	min_rating, min_rating, min_rating, min_rating
};

// unused atm but would be used for easily identifying scores on joke files
static const std::vector<float> gertrude_the_all_max_output{
	max_rating, max_rating, max_rating, max_rating,
	max_rating, max_rating, max_rating, max_rating
};

// 4k calc only
static const int num_chart_cols = 4;

// convenience loop
static const std::array<int, num_chart_cols> zto3 = { 0, 1, 2, 3 };

inline auto
downscale_low_accuracy_scores(const float f, const float sg) -> float
{
	return sg >= low_acc_cutoff
			 ? f
			 : std::min(std::max(f / powf(1.F + (low_acc_cutoff - sg), 1.25F),
								 min_rating),
						max_rating);
}

// kinda copied and pasted but also kinda use case specific
inline auto
AggregateRatings(const std::vector<float>& skillsets,
				 float rating = 0.F,
				 const float res = 10.24F,
				 const int iter = 1.F) -> float
{
	double sum;
	do {
		rating += res;
		sum = 0.0;
		for (const auto& ss : skillsets) {
			sum += std::max(0.0, 2.f / erfc(0.25 * (ss - rating)) - 2);
		}
	} while (pow(2, rating * 0.1) < sum);
	if (iter == 11)
		return rating * 1.11F;
	return AggregateRatings(skillsets, rating - res, res / 2.f, iter + 1);
}

// we want intervals not row values here, just average them, it's only for calc
// display and doesn't affect internal calculations
inline void
set_jack_diff_debug(Calc& calc, const int& hi)
{
	// interval loop
	for (int itv = 0; itv < calc.numitv; ++itv) {
		// float diff_total = 0.F;
		// int counter = 0;

		//// rows per interval now
		// for (int row = 0; row < calc.itv_jack_diff_size.at(hi).at(itv);
		// ++row) { 	diff_total += calc.jack_diff.at(hi).at(itv).at(row);
		//	++counter;
		//}

		// technically this is kind of a waste of an array but whatever
		//calc.soap.at(hi)[JackBase].at(itv) = 1.F;
	}
}
