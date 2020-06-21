#pragma once
#include <vector>
#include <string>

#include "Etterna/Models/NoteData/NoteDataStructures.h"

/* PRAISE ULBU FOR IT IS ITS GLORY THAT GIVES OUR LIVES MEANING */

static const std::string calc_params_xml = "Save/calc params.xml";
static const std::array<unsigned, num_hands> hand_col_ids = { 3, 12 };

// holds pattern mods
/*
static thread_local std::array<
  std::array<std::array<float, max_intervals>, NUM_CalcPatternMod>,
  num_hands>
  doot;
*/

inline void
Smooth(std::array<float, max_intervals>& input, float neutral, int end_interval)
{
	float f1;
	float f2 = neutral;
	float f3 = neutral;

	for (int i = 0; i < end_interval; ++i) {
		f1 = f2;
		f2 = f3;
		f3 = input.at(i);
		input.at(i) = (f1 + f2 + f3) / 3.F;
	}
}

inline void
MSSmooth(std::array<float, max_intervals>& input,
		 float neutral,
		 int end_interval)
{
	float f1;
	float f2 = neutral;

	for (int i = 0; i < end_interval; ++i) {
		f1 = f2;
		f2 = input.at(i);
		input.at(i) = (f1 + f2) / 2.F;
	}
}

static const std::vector<CalcPatternMod> agnostic_mods = {
	Stream, JS, HS, CJ, CJDensity, FlamJam, TheThing, TheThing2,
};

static const std::vector<CalcPatternMod> dependent_mods = {
	OHJumpMod,		  Balance,		 Roll,
	OHTrill,		  VOHTrill,		 Chaos,
	WideRangeBalance, WideRangeRoll, WideRangeJumptrill,
	WideRangeAnchor,  RanMan,
};

struct PatternMods
{
	static inline void set_agnostic(const CalcPatternMod& pmod,
									const float& val,
									const int& pos,
									Calc& calc)
	{
		calc.doot.at(left_hand).at(pmod).at(pos) = val;
	}

	static inline void set_dependent(const int& hand,
									 const CalcPatternMod& pmod,
									 const float& val,
									 const int& pos,
									 Calc& calc)
	{
		calc.doot.at(hand).at(pmod).at(pos) = val;
	}

	static inline void run_agnostic_smoothing_pass(const int& end_itv,
												   Calc& calc)
	{
		for (auto& pmod : agnostic_mods) {
			Smooth(calc.doot.at(left_hand).at(pmod), neutral, end_itv);
		}
	}

	static inline void run_dependent_smoothing_pass(const int& end_itv,
													Calc& calc)
	{
		for (auto& pmod : dependent_mods) {
			for (auto& h : calc.doot) {
				{
					{
						Smooth(h.at(pmod), neutral, end_itv);
					}
				}
			}
		}
	}

	static inline void bruh_they_the_same(const int& end_itv, Calc& calc)
	{
		for (auto& pmod : agnostic_mods) {
			for (int i = 0; i < end_itv; i++) {
				calc.doot.at(right_hand).at(pmod).at(i) =
				  calc.doot.at(left_hand).at(pmod).at(i);
			}
		}
	}
};
