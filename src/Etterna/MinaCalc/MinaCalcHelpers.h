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

/// unused atm but would be used for easily identifying scores on joke files
static const std::vector<float> gertrude_the_all_max_output{
	max_rating, max_rating, max_rating, max_rating,
	max_rating, max_rating, max_rating, max_rating
};

/// 4k calc only
static const int num_chart_cols = 4;

/// convenience loop
static const std::array<int, num_chart_cols> zto3 = { 0, 1, 2, 3 };

inline auto
downscale_low_accuracy_scores(const float f, const float sg) -> float
{
	return sg >= low_acc_cutoff
			 ? f
			 : std::min(std::max(f / powf(1.F + (low_acc_cutoff - sg), 3.25F),
								 min_rating),
						max_rating);
}

inline auto
aggregate_skill(const std::vector<float>& v,
				double delta_multiplier,
				float result_multiplier,
				float rating = 0.0F,
				float resolution = 10.24F) -> float
{
	// this algorithm is roughly a binary search
	// 11 iterations is enough to satisfy
	for (int i = 0; i < 11; i++) {
		double sum;

		// at least 1 repeat iteration of:
		// accumulate a sum of the input values
		//  after applying a function to the values initially
		// when threshold is reached, this iteration of the search concludes
		do {
			rating += resolution;
			sum = 0.0;
			for (const auto& vv : v) {
				sum += std::max(
				  0.0, 2.F / erfc(delta_multiplier * (vv - rating)) - 2);
			}
		} while (pow(2, rating * 0.1) < sum);

		// binary searching: move backwards and proceed forward half as quickly
		rating -= resolution;
		resolution /= 2.F;
	}
	rating += resolution * 2.F;

	return rating * result_multiplier;
}
