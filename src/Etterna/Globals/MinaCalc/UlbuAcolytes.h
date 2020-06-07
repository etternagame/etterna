#pragma once
#include <vector>
#include <string>

enum hands
{
	left_hand,
	right_hand,
	num_hands,
};

static const std::string calc_params_xml = "Save/calc params.xml";
static const std::array<unsigned, num_hands> hand_col_ids = { 3, 12 };

inline void
Smooth(vector<float>& input, float neutral)
{
	float f1;
	float f2 = neutral;
	float f3 = neutral;

	for (float& i : input) {
		f1 = f2;
		f2 = f3;
		f3 = i;
		i = (f1 + f2 + f3) / 3.F;
	}
}

inline void
Smooth(vector<vector<float>>& input, float neutral)
{
	float f1;
	float f2 = neutral;
	for (auto& itv : input) {
		for (float& i : itv) {
			f1 = f2;
			f2 = i;
			i = (f1 + f2 * 2.F) / 3.F;
		}
	}
}

inline void
DifficultyMSSmooth(vector<float>& input)
{
	float f1;
	float f2 = 0.F;

	for (float& i : input) {
		f1 = f2;
		f2 = i;
		i = (f1 + f2) / 2.F;
	}
}
