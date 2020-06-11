#pragma once
#include <array>

/* generic sequencing functions and defs to help either agnostic or dependent
 * sequencers do their stuff */
static const std::array<unsigned, 4> col_ids = { 1U, 2U, 4U, 8U };

static const float s_init = -5.F;
static const float ms_init = 5000.F;

// global multiplier to standardize baselines
static const float finalscaler = 3.632F;

// intervals are _half_ second, no point in wasting time or cpu cycles on 100
// nps joke files. even at most generous, 100 nps spread across all fingers,
// that's still 25 nps which is considerably faster than anyone can sustain
// vibro for a full second
static const int max_rows_for_single_interval = 50;

inline auto
column_count(const int& notes) -> unsigned int
{
	return notes % 2 + notes / 2 % 2 + notes / 4 % 2 + notes / 8 % 2;
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
