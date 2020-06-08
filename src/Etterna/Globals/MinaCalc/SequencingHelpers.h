#pragma once
#include <array>

/* generic sequencing functions and defs to help either agnostic or dependent
 * sequencers do their stuff */
static const std::array<unsigned, 4> col_ids = { 1U, 2U, 4U, 8U };

static const float s_init = -5.F;
static const float ms_init = 5000.F;

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

static inline auto
ms_to_scaled_nps(const float& ms) -> float
{
	float doodoo = 15000.F / ms;
	float trootroo = doodoo / 15.F;
	return trootroo * 3.632F;
}
