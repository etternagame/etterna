#pragma once

#include "Agnostic/HA_PatternMods/GenericStream.h"
#include "Agnostic/HA_PatternMods/GenericChordstream.h"
#include "Agnostic/HA_PatternMods/GenericBracketing.h"
#include "Agnostic/HA_PatternMods/CJ.h"

#include "SequencedBaseDiffCalc.h"

/// base behavior for pmod stuff
/// defines fallback for undefined keymodes
struct Bazoinkazoink
{
  public:
	Calc& _calc;
	bool dbg = false;
	int hand = 0;

	// keeps track of occurrences of basic row based sequencing, mostly for
	// skilset detection, contains itvinfo as well, the very basic metrics used
	// for detection
	metaItvInfo _mitvi;

	// meta row info keeps track of basic pattern sequencing as we scan down
	// the notedata rows, we will recyle two pointers (we want each row to be
	// able to "look back" at the meta info generated at the last row so the mhi
	// generation requires the last generated mhi object as an arg
	std::unique_ptr<metaRowInfo> _last_mri;
	std::unique_ptr<metaRowInfo> _mri;

	GStreamMod _gstream;
	GChordStreamMod _gchordstream;
	GBracketingMod _gbracketing;
	CJMod _cj;


	oversimplified_jacks lazy_jacks;

	explicit Bazoinkazoink(Calc& calc)
	  : _calc(calc)
	{
		_last_mri = std::make_unique<metaRowInfo>(calc);
		_mri = std::make_unique<metaRowInfo>(calc);
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
	}};

	const std::array<float, NUM_Skillset> basescalers = {
		0.F, 1.F, 1.F, 1.F, 0.93F, 1.F, 1.F, 1.F
	};

  public:
	virtual const std::array<std::vector<int>, NUM_Skillset>& get_pmods() const
	{
		return pmods;
	}
	virtual const std::array<float, NUM_Skillset>& get_basescalers() const
	{
		return basescalers;
	}
	virtual void adj_diff_func(
	  const size_t& itv,
	  const int& hand,
	  float*& adj_diff,
	  float*& stam_base,
	  const float& adj_npsbase,
	  const int& ss,
	  std::array<float, NUM_Skillset>& pmod_product_cur_interval)
	{
		switch (ss) {
			case Skill_Stream:
				break;
			case Skill_Jumpstream: {
			} break;
			case Skill_Handstream: {
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
	void operator()() {
		reset_base_diffs();
		hand = 0;

		full_hand_reset();
		full_agnostic_reset();
		reset_row_sequencing();

		run_agnostic_pmod_loop();
		run_dependent_pmod_loop();
	}

	virtual void full_agnostic_reset() {
		_gstream.full_reset();
		_gchordstream.full_reset();
		_gbracketing.full_reset();
		_cj.full_reset();

		_mri.get()->reset();
		_last_mri.get()->reset();
	}

	virtual void setup_agnostic_pmods() {

	}

	virtual void advance_agnostic_sequencing() {

	}

	virtual void set_agnostic_pmods(const int& itv) {
		PatternMods::set_agnostic(_gstream._pmod, _gstream(_mitvi), itv, _calc);
		PatternMods::set_agnostic(
		  _gchordstream._pmod, _gchordstream(_mitvi), itv, _calc);
		PatternMods::set_agnostic(
		  _gbracketing._pmod, _gbracketing(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_cj._pmod, _cj(_mitvi), itv, _calc);
	}

	virtual void run_agnostic_pmod_loop() {
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

	
	virtual void reset_row_sequencing() {
		_mitvi.reset();
	}

	virtual void setup_dependent_mods() {

	}

	virtual void set_dependent_pmods(const int& itv) {

	}

	virtual void full_hand_reset() {
		lazy_jacks.init(_calc.keycount);
	}

	virtual void handle_dependent_interval_end(const int& itv) {
		set_dependent_pmods(itv);

		set_sequenced_base_diffs(itv);
	}

	virtual void set_sequenced_base_diffs(const int& itv) const {

	}

	virtual void run_dependent_pmod_loop() {
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

					auto thing =
					  std::pair{ row_time,
								 ms_to_scaled_nps(
								   lazy_jacks.get_lowest_jack_ms(hand, _calc)) *
								   basescalers[Skill_JackSpeed] };
					if (std::isnan(thing.second)) {
						thing.second = 0.F;
					}
					_calc.jack_diff.at(hand).push_back(thing);
				}
			}

			hand++;
		}
	}

	/// load custom xml parameters
	virtual void load_calc_params_from_disk(bool bForce = false) const {

	}

	/// save default xml parameters
	virtual void write_params_to_disk() const {

	}

#if !defined(STANDALONE_CALC) && !defined(PHPCALC)
	static auto make_mod_param_node(
	  const std::vector<std::pair<std::string, float*>>& param_map,
	  const std::string& name) -> XNode*
	{
		auto* pmod = new XNode(name);
		for (const auto& p : param_map) {
			pmod->AppendChild(p.first, std::to_string(*p.second));
		}

		return pmod;
	}

	static void load_params_for_mod(
	  const XNode* node,
	  const std::vector<std::pair<std::string, float*>>& param_map,
	  const std::string& name)
	{
		auto boat = 0.F;
		const auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (const auto& p : param_map) {
			const auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
#endif
};
