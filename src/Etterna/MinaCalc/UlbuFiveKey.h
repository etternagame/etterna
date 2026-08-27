#pragma once

#include "UlbuBase.h"

#include "./Dependent/HD_PatternMods/HandSwitch.h"

/* my love is the great Bazoinkazoink. i love the great Five Key Ulbu. arrows are my hands, the center panel is my body, the rhythm is my heart. */

struct TheFiveEaredBazoinkazoink : public Bazoinkazoink
{
	HandSwitchMod _hsw;

	//SequencerGeneral _seq;

	diffz _diffz;

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
		  HandBalance,
		  HandSwitch,
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
			case Skill_Technical: {
				*adj_diff =
				  _calc.init_base_diff_vals.at(hand).at(TechBase).at(itv) *
				  pmod_product_cur_interval.at(ss) * basescalers.at(ss) /
				  std::max<float>(
					  fastpow(_calc.pmod_vals.at(hand).at(CJ).at(itv) + 0.05F,
							  2.F),
					  1.F);
			} break;
			default:
				break;
		}
	}

	virtual void apply_keymode_multipliers(
		std::vector<float>& cur_iteration_skillset_vals) const
	{

	}

	/// these are the base diffs which actually must be reset
	/// between calc runs or else things break
	void reset_base_diffs()
	{
		for (auto& hand : both_hands) {
			for (auto& base : {TechBase}) {
				// to be thorough: JackBase, CJBase, NPSBase, RMABase
				auto& v = _calc.init_base_diff_vals.at(hand)[base];
				std::fill(v.begin(), v.end(), 0.F);
			}
			_calc.jack_diff.at(hand).clear();
		}
	}

	/// main driver for operations
	void operator()()
	{
		reset_base_diffs();
		hand = 0;

		full_hand_reset();
		full_agnostic_reset();
		reset_row_sequencing();

		run_agnostic_pmod_loop();
		run_dependent_pmod_loop();
	}

	virtual void full_agnostic_reset()
	{
		_gchordstream.full_reset();
		_cj.full_reset();
		_hb.full_reset();

		_mri.get()->reset();
		_last_mri.get()->reset();
	}

	virtual void setup_agnostic_pmods()
	{

	}

	virtual void advance_agnostic_sequencing()
	{
		_hb.advance_sequencing(_mri->notes, _calc);
	}

	virtual void set_agnostic_pmods(const int& itv)
	{
		PatternMods::set_agnostic(
			_gchordstream._pmod, _gchordstream(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_cj._pmod, _cj(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_hb._pmod, _hb(), itv, _calc);
	}

	virtual void run_agnostic_pmod_loop()
	{
		setup_agnostic_pmods();

		for (auto itv = 0; itv < _calc.numitv; ++itv) {
			for (auto row = 0; row < _calc.itv_size.at(itv); ++row) {

				const auto& ri = _calc.adj_ni.at(itv).at(row);
				(*_mri)(
					*_last_mri, _mitvi, ri.row_time, ri.row_count, ri.row_notes);

				advance_agnostic_sequencing();

				// we only need to look back 1 metanoterow object, so we can
				// swap the one we just built into last and recycle the two
				// pointers instead of keeping track of everything
				swap(_mri, _last_mri);
			}

			// run pattern mod generation for hand agnostic mods
			set_agnostic_pmods(itv);

			// reset any accumulated interval info and set cur index number
			_mitvi.handle_interval_end();
		}

		PatternMods::run_agnostic_smoothing_pass(_calc.numitv, _calc);

		// copy left -> right for agnostic mods
		PatternMods::bruh_they_the_same(_calc.numitv, _calc);
	}


	virtual void reset_row_sequencing()
	{
		_mitvi.reset();
	}

	virtual void setup_dependent_mods()
	{

	}

	virtual void set_dependent_pmods(const int& itv)
	{
		PatternMods::set_dependent(
			hand, _gstream._pmod, _gstream(_mitvghi), itv, _calc);
		PatternMods::set_dependent(
			hand, _gbracketing._pmod, _gbracketing(_mitvghi), itv, _calc);
		PatternMods::set_dependent(
			hand, _hsw._pmod, _hsw(_mitvghi), itv, _calc);
	}

	virtual void full_hand_reset()
	{
		lazy_jacks.init(_calc.keycount);

		_gstream.full_reset();
		_gbracketing.full_reset();
		_hsw.full_reset();

		_mitvghi.zero();
		_diffz.full_reset();
	}

	void set_sequenced_base_diffs(const int& itv) const override {
		_calc.init_base_diff_vals.at(hand)[TechBase].at(itv) =
			_diffz._tc.get_itv_diff(
				_calc.init_base_diff_vals.at(hand)[NPSBase].at(itv), _calc);
	}

	virtual void handle_dependent_interval_end(const int& itv)
	{
		set_dependent_pmods(itv);

		set_sequenced_base_diffs(itv);

		_mitvghi.interval_end();
		_diffz.interval_end();
	}

	virtual void run_dependent_pmod_loop() {
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

