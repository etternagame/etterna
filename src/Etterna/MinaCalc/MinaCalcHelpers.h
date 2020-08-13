#pragma once
#include <vector>
#include <algorithm>

/* enums and other definitions that only the calcmain and its components need */

constexpr float max_rating = 100.F;
constexpr float min_rating = 0.F;
constexpr float default_score_goal = 0.93F;
constexpr float low_acc_cutoff = 0.9F;
constexpr float ssr_goal_cap = 0.965F;

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
