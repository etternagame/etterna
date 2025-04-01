#pragma once

#include "UlbuBase.h"

#include "./Dependent/HD_PatternMods/HandSwitch.h"

/* my love is the great Bazoinkazoink. i love the great Five Key Ulbu. arrows are my hands, the center panel is my body, the rhythm is my heart. */

struct TheFiveEaredBazoinkazoink : public Bazoinkazoink
{
	HandSwitchMod _hsw;

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

				*adj_diff +=
				  _calc.init_base_diff_vals.at(hand).at(NPSBase).at(itv) *
				  _calc.pmod_vals.at(hand).at(HandSwitch).at(itv);
				break;
			default:
				break;
		}
	}

	void set_dependent_pmods(const int& itv) override
	{
		PatternMods::set_dependent(
			hand, _gstream._pmod, _gstream(_mitvghi), itv, _calc);
		PatternMods::set_dependent(
			hand, _gbracketing._pmod, _gbracketing(_mitvghi), itv, _calc);
		PatternMods::set_dependent(
			hand, _hsw._pmod, _hsw(_mitvghi), itv, _calc);
	}

	void full_hand_reset() override {
		lazy_jacks.init(_calc.keycount);

		_gstream.full_reset();
		_gbracketing.full_reset();
		_hsw.full_reset();

		_mitvghi.zero();
	}

	void run_dependent_pmod_loop() override {
		setup_dependent_mods();

		hand = 0;
		for (const auto& ids : _calc.hand_col_masks) {
			full_hand_reset();
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

					// we should be advancing the sequence here, since we have the row info.
					_hsw.advance_sequencing(row_time, row_notes);

					any_ms = ms_from(row_time, last_row_time);
					auto masked_notes = row_notes & ids;

					auto non_empty_cols = find_non_empty_cols(masked_notes);
					if (non_empty_cols.empty()) {
						continue;
					}

					for (auto& c : non_empty_cols) {
						lazy_jacks(c, row_time);
					}

					// update counts
					_mitvghi.handle_row(masked_notes, ids);


					auto thing =
					  std::pair{ row_time,
								 ms_to_scaled_nps(
								   lazy_jacks.get_lowest_jack_ms(hand, _calc)) *
								   basescalers[Skill_JackSpeed] };
					if (std::isnan(thing.second)) {
						thing.second = 0.F;
					}
					_calc.jack_diff.at(hand).push_back(thing);

					last_row_time = row_time;
				}
				handle_dependent_interval_end(itv);
			}
			PatternMods::run_dependent_smoothing_pass(_calc.numitv, _calc);

			hand++;
		}
	}

#if !defined(STANDALONE_CALC) && !defined(PHPCALC)
	const std::string get_calc_param_xml() const override
	{
		return "Save/CalcParams_5k.xml";
	}

	void load_calc_params_internal(const XNode& params) const override
	{
		load_params_for_mod(&params, _cj._params, _cj.name);
		load_params_for_mod(&params, _gbracketing._params, _gbracketing.name);
		load_params_for_mod(&params, _gchordstream._params, _gchordstream.name);
		load_params_for_mod(&params, _gstream._params, _gstream.name);
		load_params_for_mod(&params, _hsw._params, _hsw.name);
	}

	XNode* make_param_node_internal(XNode* calcparams) const override
	{
		calcparams->AppendChild(
			make_mod_param_node(_cj._params, _cj.name));
		calcparams->AppendChild(
			make_mod_param_node(_gbracketing._params, _gbracketing.name));
		calcparams->AppendChild(
			make_mod_param_node(_gchordstream._params, _gchordstream.name));
		calcparams->AppendChild(
			make_mod_param_node(_gstream._params, _gstream.name));
		calcparams->AppendChild(
			make_mod_param_node(_hsw._params, _hsw.name));

		return calcparams;
	}
#endif
};

