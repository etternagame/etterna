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
		  GStream,
	  },

	  // Jumpstream
	  {
		  GChordStream,
	  },

	  // Handstream
	  {
		  GBracketing,
	  },

	  // Stamina
	  {},

	  // Jackspeed
	  {},

	  // Chordjack
	  {
		  CJ,
	  },

	  // Technical
	  {

	  },
	} };

	const std::array<float, NUM_Skillset> basescalers = {
		0.F, 1.F, 1.F, 1.F, 0.93F, 1.F, 1.F, 1.F
	};

  public:
	const std::array<std::vector<int>, NUM_Skillset>& get_pmods() const override
	{
		return pmods;
	}
	const std::array<float, NUM_Skillset>& get_basescalers() const
	{
		return basescalers;
	}

#if !defined(STANDALONE_CALC) && !defined(PHPCALC)
	const std::string get_calc_param_xml() const override
	{
		return "Save/CalcParams_6k.xml";
	}
#endif

};
