#pragma once

// stepmania garbage
#include "Etterna/Globals/global.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
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
#include "Agnostic/HA_PatternMods/FlamJam.h"
#include "Agnostic/HA_PatternMods/TheThingFinder.h"

// hand dependent data structures/functions
#include "Dependent/MetaHandInfo.h"
#include "Dependent/MetaIntervalHandInfo.h"

// hand dependent pattern mods
#include "Dependent/HD_PatternMods/OHJ.h"
#include "Dependent/HD_PatternMods/Balance.h"
#include "Dependent/HD_PatternMods/OHT.h"
#include "Dependent/HD_PatternMods/Chaos.h"
#include "Dependent/HD_PatternMods/WideRangeBalance.h"
#include "Dependent/HD_PatternMods/WideRangeRoll.h"
#include "Dependent/HD_PatternMods/WideRangeJumptrill.h"
#include "Dependent/HD_PatternMods/WideRangeAnchor.h"

// they're useful sometimes
#include "UlbuAcolytes.h"

// i am ulbu, the great bazoinkazoink in the sky
struct TheGreatBazoinkazoinkInTheSky
{
	bool dbg = false;

	// basic data we need
	vector<float>* _doots[num_hands]{};
	vector<float>* _diffs[num_hands]{};
	vector<NoteInfo> _ni;
	vector<vector<int>> _itv_rows;
	float _rate = 0.F;
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
	unique_ptr<metaRowInfo> _last_mri;
	unique_ptr<metaRowInfo> _mri;

	// tracks meta hand info as well as basic interval tracking data for hand
	// dependent stuff, like metaitvinfo and itvinfo
	metaItvHandInfo _mitvhi;

	// meta hand info is the same as meta row info, however it tracks
	// pattern progression on individual hands rather than on generic rows
	unique_ptr<metaHandInfo> _last_mhi;
	unique_ptr<metaHandInfo> _mhi;

	// i dont want to keep doing the last swap stuff every time i add something
	// new, so just put it here and pass it
	CalcWindow<float> _mw_cc_ms_any;

	// so we can make pattern mods
	StreamMod _s;
	JSMod _js;
	HSMod _hs;
	CJMod _cj;
	CJDensityMod _cjq;
	OHJumpModGuyThing _ohj;
	RollMod _roll;
	BalanceMod _bal;
	OHTrillMod _oht;
	ChaosMod _ch;
	RunningManMod _rm;
	WideRangeJumptrillMod _wrjt;
	WideRangeRollMod _wrr;
	WideRangeBalanceMod _wrb;
	WideRangeAnchorMod _wra;
	FlamJamMod _fj;
	TheThingLookerFinderThing _tt;
	TheThingLookerFinderThing2 _tt2;

	// maybe it makes sense to move generic sequencers here
	AnchorSequencer _as;

	inline void recieve_sacrifice(const vector<NoteInfo>& ni)
	{
#ifndef RELWITHDEBINFO
#if NDEBUG
		load_calc_params_from_disk();
#endif
#endif
		// ok so the problem atm is the multithreading of songload, if we
		// want to update the file on disk with new values and not just
		// overwrite it we have to write out after loading the values player
		// defined, so the quick hack solution to do that is to only do it
		// during debug output generation, which is fine for the time being,
		// though not ideal
		if (debug_lmao) {
			write_params_to_disk();
		}

		// setup our data pointers
		_last_mri = std::make_unique<metaRowInfo>();
		_mri = std::make_unique<metaRowInfo>();
		_last_mhi = std::make_unique<metaHandInfo>();
		_mhi = std::make_unique<metaHandInfo>();

		// doesn't change with offset or anything, and we may do
		// multi-passes at some point
		_ni = ni;
	}

	// for cj, will be sorted from teh above, but we dont care
	static inline auto CalcMSEstimateTWOOOOO(const vector<float>& input)
	  -> float
	{
		if (input.empty()) {
			return 1.F;
		}

		float looloo = mean(input);
		float doodoo = ms_to_bpm(looloo);
		float trootroo = doodoo / 15.F;
		return trootroo * finalscaler;
	}

	inline void heres_my_diffs(vector<float> lsoap[], vector<float> rsoap[])
	{
		_diffs[lh] = lsoap;
		_diffs[rh] = rsoap;
	}

	inline void operator()(const vector<vector<int>>& itv_rows,
						   const float& rate,
						   vector<float> ldoot[],
						   vector<float> rdoot[])
	{
		// set interval/offset pass specific stuff
		_doots[lh] = ldoot;
		_doots[rh] = rdoot;
		_itv_rows = itv_rows;
		_rate = rate;

		run_agnostic_pmod_loop();
		run_dependent_pmod_loop();
	}

#pragma region hand agnostic pmod loop
	inline void advance_agnostic_sequencing()
	{
		_fj.advance_sequencing(*_mri);
		_tt.advance_sequencing(*_mri);
		_tt2.advance_sequencing(*_mri);
	}
	inline void setup_agnostic_pmods()
	{
		// these pattern mods operate on all columns, only need basic meta
		// interval data, and do not need any more advanced pattern
		// sequencing
		for (auto& a : _doots) {
			_s.setup(a, _itv_rows.size());
			_js.setup(a, _itv_rows.size());
			_hs.setup(a, _itv_rows.size());
			_cj.setup(a, _itv_rows.size());
			_cjq.setup(a, _itv_rows.size());
			_fj.setup(a, _itv_rows.size());
			_tt.setup(a, _itv_rows.size());
			_tt2.setup(a, _itv_rows.size());
		}
	}

	inline void set_agnostic_pmods(vector<float> doot[], const int& itv)
	{
		// these pattern mods operate on all columns, only need basic meta
		// interval data, and do not need any more advanced pattern
		// sequencing just set only one hand's values and we'll copy them
		// over (or figure out how not to need to) later
		_s(_mitvi, doot);
		_js(_mitvi, doot);
		_hs(_mitvi, doot);
		_cj(_mitvi, doot);
		_cjq(_mitvi, doot);
		_fj(doot, itv);
		_tt(doot, itv);
		_tt2(doot, itv);
	}

	inline void run_agnostic_smoothing_pass(vector<float> doot[])
	{
		Smooth(doot[_s._pmod], neutral);
		Smooth(doot[_js._pmod], neutral);
		Smooth(doot[_js._pmod], neutral);
		Smooth(doot[_hs._pmod], neutral);
		Smooth(doot[_cj._pmod], neutral);
		Smooth(doot[_cjq._pmod], neutral);
		Smooth(doot[_fj._pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing::_pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing::_pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing2::_pmod], neutral);
		Smooth(doot[TheThingLookerFinderThing2::_pmod], neutral);
	}

	inline void bruh_they_the_same()
	{
		_doots[1][_s._pmod] = _doots[0][_s._pmod];
		_doots[1][_js._pmod] = _doots[0][_js._pmod];
		_doots[1][_hs._pmod] = _doots[0][_hs._pmod];
		_doots[1][_cj._pmod] = _doots[0][_cj._pmod];
		_doots[1][_cjq._pmod] = _doots[0][_cjq._pmod];
		_doots[1][_fj._pmod] = _doots[0][_fj._pmod];
		_doots[1][TheThingLookerFinderThing::_pmod] =
		  _doots[0][TheThingLookerFinderThing::_pmod];
		_doots[1][TheThingLookerFinderThing::_pmod] =
		  _doots[0][TheThingLookerFinderThing::_pmod];
		_doots[1][TheThingLookerFinderThing2::_pmod] =
		  _doots[0][TheThingLookerFinderThing2::_pmod];
		_doots[1][TheThingLookerFinderThing2::_pmod] =
		  _doots[0][TheThingLookerFinderThing2::_pmod];
	}

	inline void run_agnostic_pmod_loop()
	{
		setup_agnostic_pmods();

		// don't use s_init here, we know the first row is always 0.f and
		// therefore the first interval starts at 0.f (unless we do offset
		// passes but that's for later)
		float row_time = 0.F;
		int row_count = 0;
		unsigned row_notes = 0;

		// boop
		for (int itv = 0; itv < _itv_rows.size(); ++itv) {
			// reset any accumulated interval info and set cur index number
			_mitvi.reset(itv);

			// run the row by row construction for interval info
			for (auto& row : _itv_rows[itv]) {
				row_time = _ni[row].rowTime / _rate;
				row_notes = _ni[row].notes;
				row_count = column_count(row_notes);

				(*_mri)(*_last_mri, _mitvi, row_time, row_count, row_notes);

				advance_agnostic_sequencing();

				// we only need to look back 1 metanoterow object, so we can
				// swap the one we just built into last and recycle the two
				// pointers instead of keeping track of everything
				swap(_mri, _last_mri);
			}

			// run pattern mod generation for hand agnostic mods
			set_agnostic_pmods(_doots[lh], itv);
		}
		run_agnostic_smoothing_pass(_doots[lh]);
		bruh_they_the_same();
	}
#pragma endregion

#pragma region hand dependent pmod loop
	// some pattern mod detection builds across rows, see rm_sequencing for
	// an example, actually all sequencing should be done in objects
	// following rm_sequencing's template and be stored in mhi, and then
	// passed to whichever mods need them, but that's for later
	inline void handle_row_dependent_pattern_advancement()
	{
		_ohj.advance_sequencing(*_mhi);
		RollMod::advance_sequencing(*_mhi);
		_oht.advance_sequencing(*_mhi, _mw_cc_ms_any);
		_rm.advance_sequencing(*_mhi);
		_wrr.advance_sequencing(*_mhi);
		_wrjt.advance_sequencing(*_mhi);
		_ch.advance_sequencing(_mw_cc_ms_any);
	}

	inline void setup_dependent_mods(vector<float> _doot[])
	{
		_ohj.setup(_doot, _itv_rows.size());
		_bal.setup(_doot, _itv_rows.size());
		_roll.setup(_doot, _itv_rows.size());
		_oht.setup(_doot, _itv_rows.size());
		_ch.setup(_doot, _itv_rows.size());
		_rm.setup(_doot, _itv_rows.size());
		_wrr.setup(_doot, _itv_rows.size());
		_wrjt.setup(_doot, _itv_rows.size());
		_wrb.setup(_doot, _itv_rows.size());
		_wra.setup(_doot, _itv_rows.size());
	}

	inline void set_dependent_pmods(vector<float> doot[], const int& itv)
	{
		_ohj(_mitvhi, doot, itv);
		_bal(_mitvhi._itvhi, doot, itv);
		_roll(_mitvhi, doot, itv);
		_oht(_mitvhi._itvhi, doot, itv);
		_ch(_mitvhi._itvhi, doot, itv);
		_rm(doot, itv);
		_wrr(_mitvhi._itvhi, doot, itv);
		_wrjt(_mitvhi._itvhi, doot, itv);
		_wrb(_mitvhi._itvhi, doot, itv);
		_wra(_mitvhi._itvhi, _as, doot, itv);
	}

	inline void run_dependent_smoothing_pass(vector<float> doot[])
	{
		// need to split upohj and cjohj into 2 pmod objects
		Smooth(doot[_ohj._pmod], neutral);
		Smooth(doot[_bal._pmod], neutral);
		Smooth(doot[_roll._pmod], neutral);
		Smooth(doot[_oht._pmod], neutral);
		Smooth(doot[_ch._pmod], neutral);
		Smooth(doot[_rm._pmod], neutral);
		Smooth(doot[_wrr._pmod], neutral);
		Smooth(doot[_wrjt._pmod], neutral);
		Smooth(doot[_wrb._pmod], neutral);
		Smooth(doot[_wra._pmod], neutral);
	}

	// reset any moving windows or values when starting the other hand, this
	// shouldn't matter too much practically, but we should be disciplined
	// enough to do it anyway
	inline void full_hand_reset()
	{
		_ohj.full_reset();
		_bal.full_reset();
		_roll.full_reset();
		_oht.full_reset();
		_ch.full_reset();
		_rm.full_reset();
		_wrr.full_reset();
		_wrjt.full_reset();
		WideRangeBalanceMod::full_reset();
		WideRangeAnchorMod::full_reset();

		// zero out moving windows at the start of each hand
		_mw_cc_ms_any.zero();

		_mitvhi.zero();
		_mhi->full_reset();
		_last_mhi->full_reset();
	}

	inline void handle_dependent_interval_end(const int& itv)
	{
		// invoke metaintervalhandinfo interval end FUNCTION
		_mitvhi.interval_end();

		// test putting generic sequencers here
		_as.handle_interval_end();

		// run pattern mod generation for hand dependent mods
		set_dependent_pmods(_doots[hand], itv);
	}

	inline void run_dependent_pmod_loop()
	{
		float row_time = 0.F;
		int row_count = 0;
		int last_row_count = 0;
		int last_last_row_count = 0;
		unsigned row_notes = 0U;
		unsigned last_row_notes = 0U;
		unsigned last_last_row_notes = 0U;
		col_type ct = col_init;

		full_hand_reset();

		for (auto& ids : hand_col_ids) {
			setup_dependent_mods(_doots[hand]);

			// so we are technically doing this again (twice) and don't to
			// be doing it, but it makes debugging much less of a pita if we
			// aren't doing something like looping over intervals, running
			// agnostic pattern mods, then looping over hands for dependent
			// mods in the same interval, we may still want to do that or at
			// least have an optional set for that in case a situation
			// arises where something might need both types of info (we'd
			// also need to have 2 itvhandinfo objects, or just for general
			// performance (though the redundancy on this pass vs agnostic
			// the pass is limited to like... a couple floats and 2 ints)
			vector<float> the_simpsons;
			for (int itv = 0; itv < _itv_rows.size(); ++itv) {
				the_simpsons.clear();

				// run the row by row construction for interval info
				for (auto& row : _itv_rows[itv]) {
					row_time = _ni[row].rowTime / _rate;
					row_notes = _ni[row].notes;
					row_count = column_count(row_notes);

					ct = determine_col_type(row_notes, ids);

					// log offhand tap info (could be more performance and
					// information efficient)
					if (ct == col_empty) {

						// think itvhi wants this as well as mhi
						++_mitvhi._itvhi._offhand_taps;
						++_mhi->offhand_taps;

						if (column_count(row_notes) == 2) {
							++_mhi->offhand_ohjumps;
							++_mhi->offhand_taps;

							++_mitvhi._itvhi._offhand_taps;
						}
					}

					// only do anything else for rows with actual stuff on this
					// hand, especially the swap
					if (ct == col_empty) {
						continue;
					}

					_as(ct, row_time);

					(*_mhi)(*_last_mhi, _mw_cc_ms_any, row_time, ct, row_notes);

					bool is_cj = last_row_count > 1 && row_count > 1;

					bool was_cj = last_row_count > 1 && last_last_row_count > 1;
					bool is_scj = (row_count == 1 && last_row_count > 1) &&
								  (row_notes & last_row_notes);
					bool is_at_least_3_note_anch =
					  (row_notes & last_row_notes) & last_last_row_notes;

					// pushing back ms values, so multiply to nerf
					float pewpew = 1.25f;

					if (is_at_least_3_note_anch)
						pewpew = 1.f;

					if (is_cj || was_cj || is_scj) {

						the_simpsons.push_back(max(
						  75.F,
						  min(_mhi->cc_ms_any * pewpew, _mhi->tc_ms * pewpew)));
					}

					last_last_row_count = row_count;
					last_row_count = row_count;

					last_last_row_notes = last_row_notes;
					last_row_notes = row_notes;

					_mitvhi._itvhi.set_col_taps(ct);

					if (ct != col_init) {
						++_mitvhi._base_pattern_types[_mhi->cc];
						++_mitvhi._meta_types[_mhi->mt];
					}

					handle_row_dependent_pattern_advancement();

					std::swap(_last_mhi, _mhi);
					_mhi->offhand_ohjumps = 0;
					_mhi->offhand_taps = 0;
				}

				handle_dependent_interval_end(itv);

				_diffs[hand][BaseMS][itv] = CalcMSEstimateTWOOOOO(the_simpsons);
			}
			run_dependent_smoothing_pass(_doots[hand]);
			DifficultyMSSmooth(_diffs[hand][BaseMS]);

			// ok this is pretty jank LOL, just increment the hand index
			// when we finish left hand
			++hand;
		}
	}
#pragma endregion

	inline void load_calc_params_from_disk()
	{
		std::string fn = calc_params_xml;
		int iError;
		std::unique_ptr<RageFileBasic> pFile(
		  FILEMAN->Open(fn, RageFile::READ, iError));
		if (pFile == nullptr) {
			return;
		}

		XNode params;
		if (!XmlFileUtil::LoadFromFileShowErrors(params, *pFile)) {
			return;
		}

		// ignore params from older versions
		std::string vers;
		params.GetAttrValue("vers", vers);
		if (vers.empty() || stoi(vers) != GetCalcVersion()) {
			return;
		}

		_s.load_params_from_node(&params);
		_js.load_params_from_node(&params);
		_hs.load_params_from_node(&params);
		_cj.load_params_from_node(&params);
		_cjq.load_params_from_node(&params);
		_ohj.load_params_from_node(&params);
		_bal.load_params_from_node(&params);
		_roll.load_params_from_node(&params);
		_oht.load_params_from_node(&params);
		_ch.load_params_from_node(&params);
		_rm.load_params_from_node(&params);
		_wrr.load_params_from_node(&params);
		_wrjt.load_params_from_node(&params);
		_wrb.load_params_from_node(&params);
		_wra.load_params_from_node(&params);
		_fj.load_params_from_node(&params);
		_tt.load_params_from_node(&params);
		_tt2.load_params_from_node(&params);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* calcparams = new XNode("CalcParams");
		calcparams->AppendAttr("vers", GetCalcVersion());

		calcparams->AppendChild(_s.make_param_node());
		calcparams->AppendChild(_js.make_param_node());
		calcparams->AppendChild(_hs.make_param_node());
		calcparams->AppendChild(_cj.make_param_node());
		calcparams->AppendChild(_cjq.make_param_node());
		calcparams->AppendChild(_ohj.make_param_node());
		calcparams->AppendChild(_bal.make_param_node());
		calcparams->AppendChild(_roll.make_param_node());
		calcparams->AppendChild(_oht.make_param_node());
		calcparams->AppendChild(_ch.make_param_node());
		calcparams->AppendChild(_rm.make_param_node());
		calcparams->AppendChild(_wrr.make_param_node());
		calcparams->AppendChild(_wrjt.make_param_node());
		calcparams->AppendChild(_wrb.make_param_node());
		calcparams->AppendChild(_wra.make_param_node());
		calcparams->AppendChild(_fj.make_param_node());
		calcparams->AppendChild(_tt.make_param_node());
		calcparams->AppendChild(_tt2.make_param_node());

		return calcparams;
	}
#pragma endregion

	inline void write_params_to_disk()
	{
		std::string fn = calc_params_xml;
		std::unique_ptr<XNode> xml(make_param_node());

		std::string err;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE)) {
			return;
		}
		XmlFileUtil::SaveToFile(xml.get(), f, "", false);
	}
};
