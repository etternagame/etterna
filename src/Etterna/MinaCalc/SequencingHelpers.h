#pragma once
#include <array>
#include <algorithm>

/* generic sequencing functions and defs to help either agnostic or dependent
 * sequencers do their stuff */
static const std::array<unsigned, 4> col_ids = { 1U, 2U, 4U, 8U };

/// default for any field tracking seconds
constexpr float s_init = -5.F;
/// default for any field tracking milliseconds
constexpr float ms_init = 5000.F;

/// global multiplier to standardize baselines
constexpr float finalscaler = 3.632F * 1.06F;

inline auto
column_count(const unsigned& notes) -> int
{
	// singles
	if (notes == 1U || notes == 2U || notes == 4U || notes == 8U) {
		return 1;
	}

	// hands
	if (notes == 7U || notes == 11U || notes == 13U || notes == 14U) {
		return 3;
	}

	// quad
	if (notes == 15U) {
		return 4;
	}

	// everything else is a jump
	return 2;
}

/// milliseconds between two given timestamps in seconds
inline auto
ms_from(const float& now, const float& last) -> float
{
	return (now - last) * 1000.F;
}

/// conversion of milliseconds to bpm
inline auto
ms_to_bpm(const float& x) -> float
{
	return 15000.F / x;
}

/// conversion of milliseconds to notes per second
inline auto
ms_to_nps(const float& x) -> float
{
	return 1000.F / x;
}

/// conversion of milliseconds to notes per second scaled.
/// maybe apply final scaler later and not have this?
inline auto
ms_to_scaled_nps(const float& ms) -> float
{
	return ms_to_nps(ms) * finalscaler;
}

inline auto
max_val(const std::vector<int>& v) -> int
{
	return *std::max_element(v.begin(), v.end());
}

inline auto
max_val(const std::vector<float>& v) -> float
{
	return *std::max_element(v.begin(), v.end());
}

inline auto
max_index(const std::vector<float>& v) -> int
{
	return static_cast<int>(
	  std::distance(v.begin(), std::max_element(v.begin(), v.end())));
}

/// functions for comparing row deltas (see "any_ms" comment in
/// GenerecSequencing.h)

// it's expected that successive row deltas will often be equal,
// but this is not reliable in floating point. so any time you
// want to branch on a comparison you need to assume that very
// close values are supposed to be equal

// floats are good to around 1e-5, milliseconds scale that up by 1e3,
// if we stick to small-brain base 10 then 0.1f is all we have left

/// tolerance for comparisons of row time deltas
constexpr float any_ms_epsilon = 0.1F;

/// a > b
inline auto
any_ms_is_greater(float a, float b) -> bool
{
	return (a - b) > any_ms_epsilon;
}

/// a < b
inline auto
any_ms_is_lesser(float a, float b) -> bool
{
	return (b - a) > any_ms_epsilon;
}

/// a == b
inline auto
any_ms_is_close(float a, float b) -> bool
{
	return fabsf(a - b) <= any_ms_epsilon;
}

/// a == 0
inline auto
any_ms_is_zero(float a) -> bool
{
	return any_ms_is_close(a, 0.F);
}
