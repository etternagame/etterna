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

	inline void advance(const meta_type& mt, SequencerGeneral& _seq) {}
};

struct RollMod
{
	const CalcPatternMod _pmod = Roll;
	const std::string name = "RollMod";

#pragma region params

	float window_param = 2.F;

	float min_mod = 0.95F;
	float max_mod = 1.05F;
	float base = 0.85F;

	float cv_reset = 1.F;

	const vector<pair<std::string, float*>> _params{
		{ "window_param", &window_param },

		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },
	};
#pragma endregion params and param map

	int window = 0;
	Roll_Sequencer _roll;
	CalcMovingWindow<float> _mw_pmod;

	float moving_cv = cv_reset;
	float pmod = min_mod;

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

	inline void complete_seq() {}

	inline void advance_sequencing(const meta_type& mt, SequencerGeneral& _seq)
	{
		//_roll.advance(mt, _seq);
	}

	inline void set_pmod(const ItvHandInfo& itvhi, const SequencerGeneral& _seq)
	{
		if (itvhi.get_taps_nowi() == 0) {
			pmod = neutral;
			return;
		}

		auto loot = _seq.cv_check_sum;
		auto doot = _seq.itv_row_counter;
		float zmgoot = loot / static_cast<float>(doot + 1);

		pmod = CalcClamp(base + zmgoot, min_mod, max_mod);
	}

	inline auto operator()(const ItvHandInfo& itvhi,
						   const SequencerGeneral& _seq) -> float
	{

		set_pmod(itvhi, _seq);

		interval_end();
		_mw_pmod(pmod);
		return _mw_pmod.get_mean_of_window(window);
	}

	inline void interval_end() {}
};
#pragma once
