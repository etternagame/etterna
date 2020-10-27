#pragma once
#include <array>
#include <algorithm>

/* generic sequencing functions and defs to help either agnostic or dependent
 * sequencers do their stuff */
static const std::array<unsigned, 4> col_ids = { 1U, 2U, 4U, 8U };

constexpr float s_init = -5.F;
constexpr float ms_init = 5000.F;

// global multiplier to standardize baselines
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

inline auto
ms_from(const float& now, const float& last) -> float
{
	return (now - last) * 1000.F;
}

inline auto
ms_to_bpm(const float& x) -> float
{
	return 15000.F / x;
}

inline auto
ms_to_nps(const float& x) -> float
{
	return 1000.F / x;
}

// maybe apply final scaler later and not have this?
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
