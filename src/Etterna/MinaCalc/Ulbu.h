#pragma once

// stepmania garbage
#include "../FileTypes/XmlFile.h"
#include "../FileTypes/XmlFileUtil.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"

// hand agnostic data structures/functions
#include "Agnostic/MetaRowInfo.h"

// hand agnostic pattern mods
#include "Agnostic/HA_PatternMods/Stream.h"
#include "Agnostic/HA_PatternMods/JS.h"
#include "Agnostic/HA_PatternMods/HS.h"
#include "Agnostic/HA_PatternMods/CJ.h"
#include "Agnostic/HA_PatternMods/CJDensity.h"
#include "Agnostic/HA_PatternMods/HSDensity.h"
#include "Agnostic/HA_PatternMods/FlamJam.h"
#include "Agnostic/HA_PatternMods/TheThingFinder.h"

// hand dependent data structures/functions
#include "Dependent/MetaHandInfo.h"
#include "Dependent/MetaIntervalHandInfo.h"

// hand dependent pattern mods
#include "Dependent/HD_PatternMods/OHJ.h"
#include "Dependent/HD_PatternMods/CJOHJ.h"
#include "Dependent/HD_PatternMods/Balance.h"
#include "Dependent/HD_PatternMods/Roll.h"
#include "Dependent/HD_PatternMods/RollJS.h"
#include "Dependent/HD_PatternMods/OHT.h"
#include "Dependent/HD_PatternMods/VOHT.h"
#include "Dependent/HD_PatternMods/Chaos.h"
#include "Dependent/HD_PatternMods/CJOHAnchor.h"
#include "Dependent/HD_PatternMods/WideRangeBalance.h"
#include "Dependent/HD_PatternMods/WideRangeRoll.h"
#include "Dependent/HD_PatternMods/WideRangeJumptrill.h"
#include "Dependent/HD_PatternMods/WideRangeJJ.h"
#include "Dependent/HD_PatternMods/WideRangeAnchor.h"
#include "Dependent/HD_PatternMods/Minijack.h"
#include "Dependent/HD_PatternMods/RunningMan.h"

// they're useful sometimes
#include "UlbuAcolytes.h"

// a new thing
#include "SequencedBaseDiffCalc.h"

#include <cmath>

/** I am ulbu, the great bazoinkazoink in the sky, and ulbu does everything, for
 * ulbu is all. Praise ulbu. */
struct TheGreatBazoinkazoinkInTheSky
{
	bool dbg = false;

	Calc& _calc;
	int hand = 0;

	// to generate these

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

	// tracks meta hand info as well as basic interval tracking data for hand
	// dependent stuff, like metaitvinfo and itvinfo
	metaItvHandInfo _mitvhi;

	// meta hand info is the same as meta row info, however it tracks
	// pattern progression on individual hands rather than on generic rows
	std::unique_ptr<metaHandInfo> _last_mhi;
	std::unique_ptr<metaHandInfo> _mhi;

	SequencerGeneral _seq;

	// so we can make pattern mods with these
	StreamMod _s;
	JSMod _js;
	HSMod _hs;
	CJMod _cj;
	CJDensityMod _cjd;
	HSDensityMod _hsd;
	OHJumpModGuyThing _ohj;
	CJOHJumpMod _cjohj;
	RollMod _roll;
	RollJSMod _rolljs;
	BalanceMod _bal;
	OHTrillMod _oht;
	VOHTrillMod _voht;
	ChaosMod _ch;
	CJOHAnchorMod _chain;
	RunningManMod _rm;
	MinijackMod _mj;
	WideRangeBalanceMod _wrb;
	WideRangeRollMod _wrr;
	WideRangeJumptrillMod _wrjt;
	WideRangeJJMod _wrjj;
	WideRangeAnchorMod _wra;
	FlamJamMod _fj;
	TheThingLookerFinderThing _tt;
	TheThingLookerFinderThing2 _tt2;

	// and put them here
	PatternMods _pmods;

	// so we can apply them here
	diffz _diffz;

	explicit TheGreatBazoinkazoinkInTheSky(Calc& calc)
	  : _calc(calc)
	{
		// setup our data pointers
		_last_mri = std::make_unique<metaRowInfo>();
		_mri = std::make_unique<metaRowInfo>();
		_last_mhi = std::make_unique<metaHandInfo>();
		_mhi = std::make_unique<metaHandInfo>();
	}
	
	void operator()()
	{
		hand = 0;

		// should redundant but w.e not sure
		full_hand_reset();
		full_agnostic_reset();
		reset_row_sequencing();

		run_agnostic_pmod_loop();
		run_dependent_pmod_loop();
	}

#pragma region hand agnostic pmod loop

	void advance_agnostic_sequencing()
	{
		_s.advance_sequencing(_mri->ms_now, _mri->notes);
		_fj.advance_sequencing(_mri->ms_now, _mri->notes);
		_tt.advance_sequencing(_mri->ms_now, _mri->notes);
		_tt2.advance_sequencing(_mri->ms_now, _mri->notes);
	}

	void setup_agnostic_pmods()
	{
		/* these pattern mods operate on all columns, only need basic meta
		 * interval data, and do not need any more advanced pattern
		 * sequencing */
		_s.setup();
		_fj.setup();
		_tt.setup();
		_tt2.setup();
	}

	void full_agnostic_reset()
	{
		_s.full_reset();
		_js.full_reset();
		_hs.full_reset();
		_cj.full_reset();

		_mri.get()->reset();
		_last_mri.get()->reset();
	}

	void set_agnostic_pmods(const int& itv)
	{
		/* these pattern mods operate on all columns, only need basic meta
		 * interval data, and do not need any more advanced pattern
		 * sequencing. Just set only one hand's values and we'll copy them
		 * over (or figure out how not to need to) later */

		PatternMods::set_agnostic(_s._pmod, _s(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_js._pmod, _js(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_hs._pmod, _hs(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_cj._pmod, _cj(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_cjd._pmod, _cjd(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_hsd._pmod, _hsd(_mitvi), itv, _calc);
		PatternMods::set_agnostic(_fj._pmod, _fj(), itv, _calc);
		PatternMods::set_agnostic(_tt._pmod, _tt(), itv, _calc);
		PatternMods::set_agnostic(_tt2._pmod, _tt2(), itv, _calc);
	}

	void run_agnostic_pmod_loop()
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

#pragma endregion

#pragma region hand dependent pmod loop
	/// some pattern mod detection builds across rows, see rm_sequencing for
	/// an example, actually all sequencing should be done in objects
	/// following rm_sequencing's template and be stored in mhi, and then
	/// passed to whichever mods need them, but that's for later
	void handle_row_dependent_pattern_advancement(const float& row_time)
	{
		_ohj.advance_sequencing(_mhi->_ct, _mhi->_bt);
		_cjohj.advance_sequencing(_mhi->_ct, _mhi->_bt);
		_chain.advance_sequencing(
		  _mhi->_ct, _mhi->_bt, _mhi->_last_ct, _seq._mw_any_ms.get_now());
		_oht.advance_sequencing(_mhi->_mt, _seq._mw_any_ms);
		_voht.advance_sequencing(_mhi->_mt, _seq._mw_any_ms);
		_rm.advance_sequencing(_mhi->_ct, _mhi->_bt, _mhi->_mt, _seq._as);
		_wrr.advance_sequencing(_mhi->_bt,
								_mhi->_mt,
								_mhi->_last_mt,
								_seq._mw_any_ms.get_now(),
								_seq.get_sc_ms_now(_mhi->_ct));
		_wrjt.advance_sequencing(
		  _mhi->_bt, _mhi->_mt, _mhi->_last_mt, _seq._mw_any_ms);
		_wrjj.advance_sequencing(_mhi->_ct, row_time);
		_ch.advance_sequencing(_seq._mw_any_ms);
		_roll.advance_sequencing(_mhi->_ct, row_time);
		_rolljs.advance_sequencing(_mhi->_ct, row_time);
		_mj.advance_sequencing(_mhi->_ct, _seq.get_sc_ms_now(_mhi->_ct));
	}

	void setup_dependent_mods()
	{
		_oht.setup();
		_voht.setup();
		_roll.setup();
		_rolljs.setup();
		_rm.setup();
		_wrr.setup();
		_wrjt.setup();
		_wrjj.setup();
		_wrb.setup();
		_wra.setup();
	}

	void set_dependent_pmods(const int& itv)
	{
		PatternMods::set_dependent(hand, _ohj._pmod, _ohj(_mitvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _chain._pmod, _chain(_mitvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _cjohj._pmod, _cjohj(_mitvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _oht._pmod, _oht(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _voht._pmod, _voht(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _bal._pmod, _bal(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _roll._pmod, _roll(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _rolljs._pmod, _rolljs(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _ch._pmod, _ch(_mitvhi._itvhi.get_taps_nowi()), itv, _calc);
		PatternMods::set_dependent(
		  hand, _rm._pmod, _rm(_mitvhi._itvhi.get_taps_nowi()), itv, _calc);
		PatternMods::set_dependent(
		  hand, _wrb._pmod, _wrb(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _wrr._pmod, _wrr(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _wrjt._pmod, _wrjt(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _wrjj._pmod, _wrjj(_mitvhi._itvhi), itv, _calc);
		PatternMods::set_dependent(
		  hand, _wra._pmod, _wra(_mitvhi._itvhi, _seq._as), itv, _calc);
		PatternMods::set_dependent(
		  hand, _mj._pmod, _mj(_mitvhi._itvhi), itv, _calc);
	}

	/// reset any moving windows or values when starting the other hand, this
	/// shouldn't matter too much practically, but we should be disciplined
	/// enough to do it anyway
	void full_hand_reset()
	{
		_ohj.full_reset();
		_chain.full_reset();
		_cjohj.full_reset();
		_bal.full_reset();
		_roll.full_reset();
		_rolljs.full_reset();
		_oht.full_reset();
		_voht.full_reset();
		_ch.full_reset();
		_rm.full_reset();
		_wrr.full_reset();
		_wrjt.full_reset();
		_wrjj.full_reset();
		_wrb.full_reset();
		_wra.full_reset();
		_mj.full_reset();

		_seq.full_reset();
		_mitvhi.zero();
		_mhi->full_reset();
		_last_mhi->full_reset();
		_diffz.full_reset();
	}

	void reset_row_sequencing()
	{
		_mitvi.reset();
	}

	void handle_dependent_interval_end(const int& itv)
	{
		/* this calls itvhi's interval end, which is what updates the hand
		 * counts, so this _must_ be called before anything else */
		_mitvhi.interval_end();

		// same thing but for anchor max!!!
		_seq.interval_end();

		// run pattern mod generation for hand dependent mods
		set_dependent_pmods(itv);

		// run sequenced base difficulty generation, base diff is always
		// hand dependent so we do it in this loop
		set_sequenced_base_diffs(itv);

		_diffz.interval_end();
	}

	/// update base difficulty stuff
	void update_sequenced_base_diffs(const col_type& ct,
									 const int& itv,
									 const int& jack_counter,
									 const float& row_time,
									 const float& any_ms)
	{
		auto thing =
		  std::pair{ row_time,
					 ms_to_scaled_nps(_seq._as.get_lowest_jack_ms()) *
					   basescalers[Skill_JackSpeed] };
		if (std::isnan(thing.second)) {
			thing.second = 0.F;
		}
		// jack speed updates with highest anchor difficulty seen
		// _between either column_ for _this row_
		_calc.jack_diff.at(hand).push_back(thing);

		// debug cv stuff
		if (_calc.debugmode) {
			switch (ct) {
				case col_left:
					_calc.debugMovingWindowCV.at(hand).at(0).emplace_back(
					  row_time, _seq.get_mw_sc_ms(ct).get_cv_of_window(4));
					break;
				case col_right:
					_calc.debugMovingWindowCV.at(hand).at(1).emplace_back(
					  row_time, _seq.get_mw_sc_ms(ct).get_cv_of_window(4));
					break;
				case col_ohjump: {
					_calc.debugMovingWindowCV.at(hand).at(0).emplace_back(
					  row_time, _seq.get_mw_sc_ms(ct).get_cv_of_window(4));
					_calc.debugMovingWindowCV.at(hand).at(1).emplace_back(
					  row_time, _seq.get_mw_sc_ms(ct).get_cv_of_window(4));
					break;
				}
				default:
					break;
			}
		}

		// chordjack updates
		_diffz._cj.advance_base(any_ms, _calc);

		// tech updates with a convoluted mess of garbage
		_diffz._tc.advance_base(_seq, ct, _calc, hand, row_time);
		_diffz._tc.advance_rm_comp(_rm.get_highest_anchor_difficulty());
		_diffz._tc.advance_jack_comp(_seq._as.get_lowest_jack_ms());
	}

	void set_sequenced_base_diffs(const int& itv) const
	{
		// this is no longer done for intervals, but per row, in the row
		// (calc base anyways)
		_calc.init_base_diff_vals.at(hand)[JackBase].at(itv) =
		  _diffz._tc.get_itv_jack_diff();

		_calc.init_base_diff_vals.at(hand)[CJBase].at(itv) =
		  _diffz._cj.get_itv_diff(_calc);

		// kinda jank but includes a weighted average vs nps base to prevent
		// really silly stuff from becoming outliers
		_calc.init_base_diff_vals.at(hand)[TechBase].at(itv) =
		  _diffz._tc.get_itv_diff(
			_calc.init_base_diff_vals.at(hand)[NPSBase].at(itv), _calc);

		// mostly for debug output.. optimize later
		_calc.init_base_diff_vals.at(hand)[RMABase].at(itv) =
		  _diffz._tc.get_itv_rma_diff();
	}

	void run_dependent_pmod_loop()
	{
		setup_dependent_mods();

		for (const auto& ids : hand_col_ids) {
			auto row_time = s_init;
			auto last_row_time = s_init;
			auto any_ms = ms_init;

			auto row_notes = 0U;

			auto ct = col_init;
			full_hand_reset();

			// arrays are super bug prone with jacks so try vectors for now
			_calc.jack_diff.at(hand).clear();

			if (_calc.debugmode) {
				_calc.debugMovingWindowCV.at(hand).fill(
				  std::vector<std::pair<float, float>>());
				_calc.debugTechVals.at(hand).clear();
				_calc.debugTechVals.at(hand).shrink_to_fit();
			}

			nps::actual_cancer(_calc, hand);

			// maybe we _don't_ want this smoothed before the tech pass? and so
			// it could be constructed parallel? NEEDS TEST
			Smooth(_calc.init_base_diff_vals.at(hand).at(NPSBase), 0.F, _calc.numitv);
			MSSmooth(
			  _calc.init_base_diff_vals.at(hand).at(MSBase), 0.F, _calc.numitv);

			for (auto itv = 0; itv < _calc.numitv; ++itv) {
				auto jack_counter = 0;
				for (auto row = 0; row < _calc.itv_size.at(itv); ++row) {

					const auto& ri = _calc.adj_ni.at(itv).at(row);
					row_time = ri.row_time;
					row_notes = ri.row_notes;
					const auto row_count = ri.row_count;

					// don't like having this here
					any_ms = ms_from(row_time, last_row_time);

					// To catch division by 0, not preventing significant issues as-is
					//	So disabled assert for now
					// assert(any_ms > 0.F);

					ct = determine_col_type(row_notes, ids);

					// cj must always update
					_diffz._cj.update_flags(row_notes, row_count);

					// handle any special cases that need to be executed on
					// empty rows for this hand here before moving on, aside
					// from whatever is in this block _nothing_ else should
					// update unless there is a note to update with
					if (ct == col_empty) {
						_rm.advance_off_hand_sequencing();
						_mj.advance_off_hand_sequencing();
						if (row_count == 2) {
							_rm.advance_off_hand_sequencing();
						}
						continue;
					}

					// basically a time master, keeps track of different
					// timings, update first
					_seq.advance_sequencing(ct, row_time, any_ms);

					// update metahandinfo, it constructs basic and advanced
					// patterns from where we are now + recent pattern
					// information constructed by the last iteration of
					// itself
					(*_mhi)(*_last_mhi, ct);

					// update interval aggregation of column taps
					_mitvhi._itvhi.set_col_taps(ct);

					// advance sequencing for all hand dependent mods
					handle_row_dependent_pattern_advancement(row_time);

					/* jackspeed, and tech use various adjust ms bases that
					 * are sequenced here, meaning they are order dependent
					 * (jack might not be for the moment actually) nps base
					 * is still calculated in the old way */
					update_sequenced_base_diffs(
					  ct, itv, jack_counter, row_time, any_ms);
					++jack_counter;

					// only ohj uses this atm (and probably into the future)
					// so it might kind of be a waste?
					if (_mhi->_bt != base_type_init) {
						++_mitvhi._base_types.at(_mhi->_bt);
						++_mitvhi._meta_types.at(_mhi->_mt);
					}

					// cycle the pointers so now becomes last
					std::swap(_last_mhi, _mhi);
					last_row_time = row_time;
				}

				// maybe this should go back into the diffz object...
				// _calc->itv_jack_diff_size.at(hand).at(itv) = jack_counter;

				handle_dependent_interval_end(itv);
			}
			PatternMods::run_dependent_smoothing_pass(_calc.numitv, _calc);
			//Smooth(_calc.init_base_diff_vals.at(hand).at(CJBase), 0.F, _calc.numitv);

			// ok this is pretty jank LOL, just increment the hand index
			// when we finish left hand
			++hand;
		}

		nps::grindscale(_calc);
	}
#pragma endregion

	[[nodiscard]] static auto make_mod_param_node(
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

	void load_calc_params_from_disk(bool bForce = false) const
	{
		const auto fn = calc_params_xml;
		int iError;

		// Hold calc params program-global persistent info
		thread_local RageFileBasic* pFile;
		thread_local XNode params;
		// Only ever try to load params once per thread unless forcing
		thread_local bool paramsLoaded = false;

		// Don't keep loading params if nothing to load/no reason to
		// Allow a force to bypass
		if (paramsLoaded && !bForce)
			return;

		// Load if missing
		if (pFile == nullptr || bForce) {
			delete pFile;
			pFile = FILEMAN->Open(fn, RageFile::READ, iError);
			// Failed to load
			if (pFile == nullptr)
				return;
		}

		// If it isn't loaded or we are forcing a load, load it
		if (params.ChildrenEmpty() || bForce)
		{
			if (!XmlFileUtil::LoadFromFileShowErrors(params, *pFile)) {
				return;
			}
		}

		// ignore params from older versions
		std::string vers;
		params.GetAttrValue("vers", vers);
		if (vers.empty() || stoi(vers) != GetCalcVersion()) {
			return;
		}
		paramsLoaded = true;

		// diff params
		load_params_for_mod(&params, _diffz._cj._params, _diffz._cj.name);
		load_params_for_mod(&params, _diffz._tc._params, _diffz._tc.name);

		// pmods
		load_params_for_mod(&params, _s._params, _s.name);
		load_params_for_mod(&params, _js._params, _js.name);
		load_params_for_mod(&params, _hs._params, _hs.name);
		load_params_for_mod(&params, _cj._params, _cj.name);
		load_params_for_mod(&params, _cjd._params, _cjd.name);
		load_params_for_mod(&params, _hsd._params, _hsd.name);
		load_params_for_mod(&params, _ohj._params, _ohj.name);
		load_params_for_mod(&params, _cjohj._params, _cjohj.name);
		load_params_for_mod(&params, _chain._params, _chain.name);
		load_params_for_mod(&params, _bal._params, _bal.name);
		load_params_for_mod(&params, _oht._params, _oht.name);
		load_params_for_mod(&params, _voht._params, _voht.name);
		load_params_for_mod(&params, _ch._params, _ch.name);
		load_params_for_mod(&params, _rm._params, _rm.name);
		load_params_for_mod(&params, _roll._params, _roll.name);
		load_params_for_mod(&params, _rolljs._params, _rolljs.name);
		load_params_for_mod(&params, _wrb._params, _wrb.name);
		load_params_for_mod(&params, _wrr._params, _wrr.name);
		load_params_for_mod(&params, _wrjt._params, _wrjt.name);
		load_params_for_mod(&params, _wrjj._params, _wrjj.name);
		load_params_for_mod(&params, _wra._params, _wra.name);
		load_params_for_mod(&params, _mj._params, _mj.name);
		load_params_for_mod(&params, _fj._params, _fj.name);
		load_params_for_mod(&params, _tt._params, _tt.name);
		load_params_for_mod(&params, _tt2._params, _tt2.name);
	}

	[[nodiscard]] auto make_param_node() const -> XNode*
	{
		auto* calcparams = new XNode("CalcParams");
		calcparams->AppendAttr("vers", GetCalcVersion());

		// diff params
		calcparams->AppendChild(
		  make_mod_param_node(_diffz._cj._params, _diffz._cj.name));
		calcparams->AppendChild(
		  make_mod_param_node(_diffz._tc._params, _diffz._tc.name));

		// pmods
		calcparams->AppendChild(make_mod_param_node(_s._params, _s.name));
		calcparams->AppendChild(make_mod_param_node(_js._params, _js.name));
		calcparams->AppendChild(make_mod_param_node(_hs._params, _hs.name));
		calcparams->AppendChild(make_mod_param_node(_cj._params, _cj.name));
		calcparams->AppendChild(make_mod_param_node(_cjd._params, _cjd.name));
		calcparams->AppendChild(make_mod_param_node(_hsd._params, _hsd.name));
		calcparams->AppendChild(make_mod_param_node(_ohj._params, _ohj.name));
		calcparams->AppendChild(
		  make_mod_param_node(_cjohj._params, _cjohj.name));
		calcparams->AppendChild(
		  make_mod_param_node(_chain._params, _chain.name));
		calcparams->AppendChild(make_mod_param_node(_bal._params, _bal.name));
		calcparams->AppendChild(make_mod_param_node(_oht._params, _oht.name));
		calcparams->AppendChild(make_mod_param_node(_voht._params, _voht.name));
		calcparams->AppendChild(make_mod_param_node(_ch._params, _ch.name));
		calcparams->AppendChild(make_mod_param_node(_rm._params, _rm.name));
		calcparams->AppendChild(make_mod_param_node(_roll._params, _roll.name));
		calcparams->AppendChild(make_mod_param_node(_rolljs._params, _rolljs.name));
		calcparams->AppendChild(make_mod_param_node(_wrb._params, _wrb.name));
		calcparams->AppendChild(make_mod_param_node(_wrr._params, _wrr.name));
		calcparams->AppendChild(make_mod_param_node(_wrjt._params, _wrjt.name));
		calcparams->AppendChild(make_mod_param_node(_wrjj._params, _wrjj.name));
		calcparams->AppendChild(make_mod_param_node(_wra._params, _wra.name));
		calcparams->AppendChild(make_mod_param_node(_mj._params, _mj.name));
		calcparams->AppendChild(make_mod_param_node(_fj._params, _fj.name));
		calcparams->AppendChild(make_mod_param_node(_tt._params, _tt.name));
		calcparams->AppendChild(make_mod_param_node(_tt2._params, _tt2.name));

		return calcparams;
	}
#pragma endregion

	void write_params_to_disk() const
	{
		const auto fn = calc_params_xml;
		const std::unique_ptr<XNode> xml(make_param_node());

		std::string err;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE)) {
			return;
		}
		XmlFileUtil::SaveToFile(xml.get(), f, "", false);
	}
};
