#pragma once
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <xmmintrin.h>
#include <numeric>

/* generic pattern mod functions and defs to help either agnostic or dependent
 * mods do their stuff */
constexpr float neutral = 1.F;

/// Relies on endiannes (significantly inaccurate)
inline auto
fastpow(double a, double b) -> float
{
	int u[2];
	std::memcpy(&u, &a, sizeof a);
	u[1] = static_cast<int>(b * (u[1] - 1072632447) + 1072632447);
	u[0] = 0;
	std::memcpy(&a, &u, sizeof a);
	return static_cast<float>(a);
}

/// not super accurate, good enough for our purposes
inline auto
fastsqrt(float _in) -> float
{
	if (_in == 0.F) {
		return 0.F;
	}
	const auto in = _mm_load_ss(&_in);
	float out;
	_mm_store_ss(&out, _mm_mul_ss(in, _mm_rsqrt_ss(in)));
	return out;
}

template<typename T>
auto
sum(const std::vector<T>& v) -> T
{
	return std::accumulate(begin(v), end(v), static_cast<T>(0));
}

template<typename T>
auto
mean(const std::vector<T>& v) -> float
{
	return static_cast<float>(sum(v)) / static_cast<float>(v.size());
}

// Coefficient of variation
inline auto
cv(const std::vector<float>& input) -> float
{
	auto sd = 0.F;
	const auto average = mean(input);
	for (auto i : input) {
		sd += (i - average) * (i - average);
	}

	return fastsqrt(sd / static_cast<float>(input.size())) / average;
}

// cv of a vector truncated to a set number of values, or if below, filled with
// dummy values to reach the desired num_vals
inline auto
cv_trunc_fill(const std::vector<float>& input,
			  const int& num_vals,
			  const float& ms_dummy) -> float
{
	int input_sz = static_cast<int>(input.size());
	float sd = 0.F;
	float average = 0.F;
	if (input_sz >= num_vals) {
		for (int i = 0; i < std::min(input_sz, num_vals); ++i) {
			average += input[i];
		}
		average /= static_cast<float>(num_vals);

		for (int i = 0; i < std::min(input_sz, num_vals); ++i) {
			sd += (input[i] - average) * (input[i] - average);
		}

		return fastsqrt(sd / static_cast<float>(num_vals)) / average;
	}

	for (int i = 0; i < std::min(input_sz, num_vals); ++i) {
		average += input[i];
	}

	// fill with dummies if input is below desired number of values
	for (int i = 0; i < num_vals - input_sz; ++i) {
		average += ms_dummy;
	}
	average /= static_cast<float>(num_vals);

	for (int i = 0; i < std::min(input_sz, num_vals); ++i) {
		sd += (input[i] - average) * (input[i] - average);
	}

	for (int i = 0; i < num_vals - input_sz; ++i) {
		sd += (ms_dummy - average) * (ms_dummy - average);
	}

	return fastsqrt(sd / static_cast<float>(num_vals)) / average;
}

inline auto
sum_trunc_fill(const std::vector<float>& input,
			   const int& num_vals,
			   const float& ms_dummy) -> float
{
	int input_sz = static_cast<int>(input.size());
	float sum = 0.F;
	// use up to num_vals
	for (int i = 0; i < std::min(input_sz, num_vals); ++i) {
		sum += input[i];
	}

	// got enough
	if (input_sz >= num_vals) {
		return sum;
	}

	// fill with dummies if input is below desired number of values
	for (int i = 0; i < num_vals - static_cast<int>(input_sz); ++i) {
		sum += ms_dummy;
	}

	return sum;
}

inline auto
div_high_by_low(float a, float b) -> float
{
	if (b > a) {
		std::swap(a, b);
	}
	return a / b;
}

inline auto
div_low_by_high(float a, float b) -> float
{
	if (b > a) {
		std::swap(a, b);
	}
	return b / a;
}

inline auto
diff_high_by_low(int a, int b) -> int
{
	if (b > a) {
		std::swap(a, b);
	}
	return a - b;
}

inline auto
weighted_average(const float& a, const float& b, const float& x, const float& y)
  -> float
{
	return (x * a + ((y - x) * b)) / y;
}

inline auto
lerp(float t, float a, float b) -> float
{
	return (1.F - t)*a + t*b;
}
