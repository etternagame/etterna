#pragma once

#include "UlbuBase.h"

#include "Dependent/HD_PatternMods/HandSwitch.h"

/* my love is the great Bazoinkazoink. i love the great Five Key Ulbu. arrows are my hands, the center panel is my body, the rhythm is my heart. */

struct TheFiveEaredBazoinkazoink : public Bazoinkazoink
{
	explicit TheFiveEaredBazoinkazoink(Calc& calc)
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
		  HandSwitch
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
	const std::array<float, NUM_Skillset>& get_basescalers() const override
	{
		return basescalers;
	}
	void adj_diff_func(
	  const size_t& itv,
	  const int& hand,
	  float*& adj_diff,
	  float*& stam_base,
	  const float& adj_npsbase,
	  const int& ss,
	  std::array<float, NUM_Skillset>& pmod_product_cur_interval) override
	{
		switch (ss) {
			case Skill_Stream:
				break;
			case Skill_Jumpstream: {
				auto a = *adj_diff;
				auto b =
				  _calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv) *
				  pmod_product_cur_interval[Skill_Handstream];
				*stam_base = std::max<float>(a, b);
			} break;
			case Skill_Handstream: {
				auto a = adj_npsbase;
				auto b =
				  _calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv) *
				  pmod_product_cur_interval[Skill_Jumpstream];
				*stam_base = std::max<float>(a, b);
			} break;
			case Skill_JackSpeed:
				break;
			case Skill_Chordjack:
				break;
			case Skill_Technical:
				*adj_diff =
				  _calc.init_base_diff_vals.at(hand).at(TechBase).at(itv) *
				  pmod_product_cur_interval.at(ss) * basescalers.at(ss);
				break;
			default:
				break;
		}
	}

#if !defined(STANDALONE_CALC) && !defined(PHPCALC)
	const std::string get_calc_param_xml() const override
	{
		return "Save/CalcParams_5k.xml";
	}
#endif
};

