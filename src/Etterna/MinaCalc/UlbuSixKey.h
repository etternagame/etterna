#pragma once

#include "UlbuBase.h"

struct TheSixEyedBazoinkazoink : public Bazoinkazoink
{
	explicit TheSixEyedBazoinkazoink(Calc& calc)
	  : Bazoinkazoink(calc)
	{

	}

  private:
	const std::array<std::vector<int>, NUM_Skillset> pmods = { {
	  // Overall
	  {},

	  // Stream
	  {

	  },

	  // Jumpstream
	  {

	  },

	  // Handstream
	  {

	  },

	  // Stamina
	  {},

	  // Jackspeed
	  {},

	  // Chordjack
	  {

	  },

	  // Technical
	  {

	  },
	} };

  public:
	const std::array<std::vector<int>, NUM_Skillset>& get_pmods() const override
	{
		return pmods;
	}

	void operator()() override
	{
		reset_base_diffs();

		// just nps base
		unsigned hand = 0;
		for (const auto& ids : _calc.hand_col_masks) {
			nps::actual_cancer(_calc, hand);
			Smooth(_calc.init_base_diff_vals.at(hand).at(NPSBase),
				   0.F,
				   _calc.numitv);

			hand++;
		}

	}

	void load_calc_params_from_disk(bool bForce = false) const override {

	}

	void write_params_to_disk() const override {

	}

};
