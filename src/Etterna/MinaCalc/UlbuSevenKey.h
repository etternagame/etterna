#pragma once

#include "UlbuBase.h"

struct TheSevenFootedBazoinkazoink : public Bazoinkazoink
{
	explicit TheSevenFootedBazoinkazoink(Calc& calc)
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
	const std::array<std::vector<int>, NUM_Skillset> get_pmods() const override
	{
		return pmods;
	}

	void operator()() override {

	}

	void load_calc_params_from_disk(bool bForce = false) const override {

	}

	void write_params_to_disk() const override {

	}

};
