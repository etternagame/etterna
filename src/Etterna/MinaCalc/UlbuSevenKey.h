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

	void operator()() override
	{
		reset_base_diffs();

		lazy_jacks.init(_calc.keycount);

		// just nps base
		unsigned hand = 0;
		for (const auto& ids : _calc.hand_col_masks) {
			nps::actual_cancer(_calc, hand);
			Smooth(_calc.init_base_diff_vals.at(hand).at(NPSBase),
				   0.F,
				   _calc.numitv);

			auto row_time = s_init;
			auto last_row_time = s_init;
			auto any_ms = ms_init;
			auto row_notes = 0u;
			for (auto itv = 0; itv < _calc.numitv; ++itv) {
				for (auto row = 0; row < _calc.itv_size.at(itv); row++) {
					const auto& ri = _calc.adj_ni.at(itv).at(row);
					row_time = ri.row_time;
					row_notes = ri.row_notes;
					any_ms = ms_from(row_time, last_row_time);
					auto masked_notes = row_notes & ids;

					auto non_empty_cols = find_non_empty_cols(masked_notes);
					if (non_empty_cols.empty()) {
						continue;
					}

					for (auto& c : non_empty_cols) {
						lazy_jacks(c, row_time);
					}

					auto thing = std::pair{
						row_time,
						ms_to_scaled_nps(lazy_jacks.get_lowest_jack_ms(hand, _calc)) *
						  basescalers[Skill_JackSpeed]
					};
					if (std::isnan(thing.second)) {
						thing.second = 0.F;
					}
					_calc.jack_diff.at(hand).push_back(thing);
				}
			}

			hand++;
		}

	}

	void load_calc_params_from_disk(bool bForce = false) const override {

	}

	void write_params_to_disk() const override {

	}

};
