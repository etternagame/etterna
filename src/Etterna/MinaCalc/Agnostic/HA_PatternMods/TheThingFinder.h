#pragma once
#include "../../PatternModHelpers.h"
#include "../HA_Sequencers/ThingSequencing.h"

// the a things, they are there, we must find them...
// probably add a timing check to this as well
struct TheThingLookerFinderThing
{
	const CalcPatternMod _pmod = TheThing;
	const std::string name = "TheThingMod";

#pragma region params

	float min_mod = 0.15F;
	float max_mod = 1.F;
	float base = 0.05F;

	// params for tt_sequencing
	float group_tol = 35.F;
	float step_tol = 17.5F;
	float scaler = 0.2F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		//{ "group_tol", &group_tol },
		//{ "step_tol", &step_tol },
		{ "scaler", &scaler },
	};
#pragma endregion params and param map

	// sequencer
	TT_Sequencing tt;
	float pmod = min_mod;

#pragma region generic functions
	void setup() { tt.set_params(group_tol, step_tol, scaler); }

#pragma endregion

	void advance_sequencing(const float& ms_now, const unsigned& notes)
	{
		tt(ms_now, notes);
	}

	auto operator()() -> float
	{
		pmod =
		  tt.mod_parts[0] + tt.mod_parts[1] + tt.mod_parts[2] + tt.mod_parts[3];
		pmod /= 4.F;
		pmod = std::clamp(base + pmod, min_mod, max_mod);

		// reset flags n stuff
		tt.reset();

		return pmod;
	}
};

// the a things, they are there, we must find them...
// probably add a timing check to this as well
struct TheThingLookerFinderThing2
{
	const CalcPatternMod _pmod = TheThing2;
	const std::string name = "TheThing2Mod";

#pragma region params

	float min_mod = 0.15F;
	float max_mod = 1.F;
	float base = 0.05F;

	// params for tt_sequencing
	float group_tol = 35.F;
	float step_tol = 17.5F;
	float scaler = 0.2F;

	const std::vector<std::pair<std::string, float*>> _params{
		{ "min_mod", &min_mod },
		{ "max_mod", &max_mod },
		{ "base", &base },

		//{ "group_tol", &group_tol },
		//{ "step_tol", &step_tol },
		{ "scaler", &scaler },
	};
#pragma endregion params and param map

	// sequencer
	TT_Sequencing2 tt2;
	float pmod = min_mod;

#pragma region generic functions
	void setup() { tt2.set_params(group_tol, step_tol, scaler); }

#pragma endregion

	void advance_sequencing(const float& ms_now, const unsigned& notes)
	{
		tt2(ms_now, notes);
	}

	auto operator()() -> float
	{
		pmod = tt2.mod_parts[0] + tt2.mod_parts[1] + tt2.mod_parts[2] +
			   tt2.mod_parts[3];
		pmod /= 4.F;
		pmod = std::clamp(base + pmod, min_mod, max_mod);

		// reset flags n stuff
		tt2.reset();

		return pmod;
	}
};
