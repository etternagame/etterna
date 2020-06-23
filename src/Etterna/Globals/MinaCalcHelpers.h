#pragma once
#include <vector>

// enums and other definitions that won't change often

static const vector<float> dimples_the_all_zero_output{ 0.F, 0.F, 0.F, 0.F,
														0.F, 0.F, 0.F, 0.F };
static const vector<float> gertrude_the_all_max_output{ 100.F, 100.F, 100.F,
														100.F, 100.F, 100.F,
														100.F, 100.F };

static const int num_chart_cols = 4;

static const std::array<int, num_chart_cols> zto3 = { 0, 1, 2, 3 };

inline auto
downscale_low_accuracy_scores(float f, float sg) -> float
{
	return sg >= 0.93F
			 ? f
			 : min(max(f / pow(1.F + (0.93F - sg), 1.25F), 0.F), 100.F);
}

inline auto
AggregateScores(const vector<float>& skillsets, float rating, float resolution)
  -> float
{
	float sum;
	for (int iter = 1; iter <= 11; iter++) {
		do {
			rating += resolution;
			sum = 0.0F;
			for (float i : skillsets) {
				sum += 2.F / std::erfc(0.5F * (i - rating)) - 1.F;
			}
		} while (3 < sum);
		rating -= resolution;
		resolution /= 2.F;
	}
	return rating + 2.F * resolution;
}
