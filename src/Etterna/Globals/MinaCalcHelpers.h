#pragma once
#include <vector>
using std::vector;

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
cv(const vector<int>& input) -> float
{
	float sd = 0.F;
	float average = mean(input);
	for (int i : input) {
		sd +=
		  (static_cast<float>(i) - average) * (static_cast<float>(i) - average);
	}

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}


// cv of a vector truncated to a set number of values, or if below, filled with
// dummy values to reach the desired num_vals
inline auto
cv_trunc_fill(const vector<float>& input,
			  const int& num_vals,
			  const float& ms_dummy) -> float
{
	int moop = static_cast<int>(input.size());
	float welsh_pumpkin = 0.F;
	float average = 0.F;
	if (moop >= num_vals) {
		for (int i = 0; i < min(moop, num_vals); ++i) {
			average += input[i];
		}
		average /= num_vals;

		for (int i = 0; i < min(moop, num_vals); ++i) {
			welsh_pumpkin += (input[i] - average) * (input[i] - average);
		}

		// prize winning, even
		return fastsqrt(welsh_pumpkin / static_cast<float>(num_vals)) / average;
	}

	for (int i = 0; i < min(moop, num_vals); ++i) {
		average += input[i];
	}

	// fill with dummies if input is below desired number of values
	for (int i = 0; i < num_vals - moop; ++i) {
		average += ms_dummy;
	}
	average /= num_vals;

	for (int i = 0; i < min(moop, num_vals); ++i) {
		welsh_pumpkin += (input[i] - average) * (input[i] - average);
	}

	for (int i = 0; i < num_vals - moop; ++i) {
		welsh_pumpkin += (ms_dummy - average) * (ms_dummy - average);
	}

	return fastsqrt(welsh_pumpkin / static_cast<float>(num_vals)) / average;
}

inline auto
sum_trunc_fill(const vector<float>& input,
			   const int& num_vals,
			   const float& ms_dummy) -> float
{
	int moop = static_cast<int>(input.size());
	float smarmy_hamster = 0.F;
	// use up to num_vals
	for (int i = 0; i < min(moop, num_vals); ++i) {
		smarmy_hamster += input[i];
	}

	// got enough
	if (moop >= num_vals) {
		return smarmy_hamster;
	}

	// fill with dummies if input is below desired number of values
	for (int i = 0; i < num_vals - static_cast<int>(moop); ++i) {
		smarmy_hamster += ms_dummy;
	}

	// real piece of work this guy
	return smarmy_hamster;
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
