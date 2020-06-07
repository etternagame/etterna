#pragma once
#include <array>
#include <vector>

#include "Etterna/Globals/MinaCalcHelpers.h"

using std::vector;

// for cj, will be sorted from teh above, but we dont care
static inline auto
CJBaseDifficultySequencing(const vector<float>& input) -> float
{
	if (input.empty()) {
		return 1.F;
	}

	float looloo = mean(input);
	float doodoo = ms_to_bpm(looloo);
	float trootroo = doodoo / 15.F;
	return trootroo * finalscaler;
}



static inline auto
TechBaseDifficultySequencing(const vector<float>& input) -> float
{
	if (input.empty()) {
		return 1.F;
	}

	float looloo = mean(input);
	float doodoo = ms_to_bpm(looloo);
	float trootroo = doodoo / 15.F;
	return trootroo * finalscaler;
}
