#pragma once
#include <string>
#include <array>
#include <vector>

using std::pair;
using std::vector;

#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "Etterna/Globals/MinaCalc/Dependent/IntervalHandInfo.h"

static const int max_seq_parts = 4;

// contains parts of a modifier constructed from sequences
struct mod_parts
{
	std::array<int, max_seq_parts> _parts = { 0, 0, 0, 0 };
};

// contains sequence
struct RollSeq
{
};

// update logic
struct Roll_Sequencer
{
	RollSeq _rs;

	inline void advance(const meta_type& mt, SequencerGeneral& _seq)
	{

	}
};

struct RollMod
{
	const CalcPatternMod _pmod = Roll;
	const std::string name = "RollMod";

#pragma region params

	float window_param = 2.F;

	float min_mod = 0.25F;
	float max_mod = 1.F;
	float base = 0.15F;
	float scaler = 0.9F;

	float cv_reset = 1.F;
	float cv_threshold = 0.35F;
	float other_cv_threshold = 0.3F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
		{ "scaler", &scaler },

		{ "cv_reset", &cv_reset },
		{ "cv_threshold", &cv_threshold },
		{ "other_cv_threshold", &other_cv_threshold },
	};
#pragma endregion params and param map

	int window = 0;
	Roll_Sequencer _roll;
	CalcMovingWindow<float> _mw_pmod;

	float moving_cv = cv_reset;
	float pmod = min_mod;

#pragma region generic functions

	inline void full_reset()
	{
		moving_cv = (moving_cv + cv_reset);
		pmod = neutral;
	}

	inline void setup()
	{
		window =
		  CalcClamp(static_cast<int>(window_param), 1, max_moving_window_size);
	}

#pragma endregion

	inline void complete_seq() {}

	inline void advance_sequencing(const meta_type& mt, SequencerGeneral& _seq)
	{
		_roll.advance(mt, _seq);
	}

	inline void set_pmod(const ItvHandInfo& itvhi) {}

	inline auto operator()(const ItvHandInfo& itvhi, const SequencerGeneral& _seq)
	  -> float
	{
		auto loot = _seq.cv_check_sum;
		auto doot = _seq.itv_row_counter;
		float zmgoot = loot / static_cast<float>(doot + 1);
		pmod = 0.5f + zmgoot;
		//set_pmod(itvhi);

		interval_end();
		return pmod;
	}

	inline void interval_end() {}
};
#pragma once
