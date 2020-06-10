#pragma once
#include <vector>

// enums and other definitions that won't change often

// intervals are _half_ second, no point in wasting time or cpu cycles on 100
// nps joke files
static const int max_nps_for_single_interval = 50;
static const vector<float> dimples_the_all_zero_output{ 0.F, 0.F, 0.F, 0.F,
														0.F, 0.F, 0.F, 0.F };
static const vector<float> gertrude_the_all_max_output{ 100.F, 100.F, 100.F,
														100.F, 100.F, 100.F,
														100.F, 100.F };
// DON'T WANT TO RECOMPILE HALF THE GAME IF I EDIT THE HEADER FILE
// global multiplier to standardize baselines
static const float finalscaler = 3.632F;

static const int num_chart_cols = 4;

static const int zto3[4] = { 0, 1, 2, 3 };

inline auto
ms_to_bpm(float x) -> float
{
	return 15000.F / x;
}

inline auto
downscale_low_accuracy_scores(float f, float sg) -> float
{
	return sg >= 0.93F
			 ? f
			 : min(max(f / pow(1.F + (0.93F - sg), 0.75F), 0.F), 100.F);
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

inline auto
max_val(vector<int>& v) -> int
{
	return *std::max_element(v.begin(), v.end());
}

inline auto
max_val(vector<float>& v) -> float
{
	return *std::max_element(v.begin(), v.end());
}

inline auto
max_index(vector<float>& v) -> int
{
	return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}
