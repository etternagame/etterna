#pragma once

#include <deque>

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
#include "Dependent/HD_PatternMods/Roll.h"
#include "Dependent/HD_PatternMods/OHT.h"
#include "Dependent/HD_PatternMods/Chaos.h"
#include "Dependent/HD_PatternMods/WideRangeBalance.h"
#include "Dependent/HD_PatternMods/WideRangeRoll.h"
#include "Dependent/HD_PatternMods/WideRangeJumptrill.h"
#include "Dependent/HD_PatternMods/WideRangeAnchor.h"
#include "Dependent/HD_PatternMods/RunningMan.h"

// they're useful sometimes
#include "UlbuAcolytes.h"

// a new thing
#include "Etterna/Globals/MinaCalc/SequencedBaseDiffCalc.h"

// actual cancer
bool debug_lmao;

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

	SequencerGeneral _seq;

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
	WideRangeBalanceMod _wrb;
	WideRangeRollMod _wrr;
	WideRangeJumptrillMod _wrjt;
	WideRangeAnchorMod _wra;
	FlamJamMod _fj;
	TheThingLookerFinderThing _tt;
	TheThingLookerFinderThing2 _tt2;

	inline void allocate_doot()
	{
		// agnostics
		_doots[left_hand][_s._pmod].resize(_itv_rows.size());
		_doots[left_hand][_js._pmod].resize(_itv_rows.size());
		_doots[left_hand][_hs._pmod].resize(_itv_rows.size());
		_doots[left_hand][_cj._pmod].resize(_itv_rows.size());
		_doots[left_hand][_cjq._pmod].resize(_itv_rows.size());
		_doots[left_hand][_fj._pmod].resize(_itv_rows.size());
		_doots[left_hand][_tt._pmod].resize(_itv_rows.size());
		_doots[left_hand][_tt2._pmod].resize(_itv_rows.size());

		// dependents
		for (auto& h : { left_hand, right_hand }) {
			_doots[h][_ohj._pmod].resize(_itv_rows.size());
			_doots[h][_bal._pmod].resize(_itv_rows.size());
			_doots[h][_roll._pmod].resize(_itv_rows.size());
			_doots[h][_oht._pmod].resize(_itv_rows.size());
			_doots[h][_ch._pmod].resize(_itv_rows.size());
			_doots[h][_rm._pmod].resize(_itv_rows.size());
			_doots[h][_wrb._pmod].resize(_itv_rows.size());
			_doots[h][_wrr._pmod].resize(_itv_rows.size());
			_doots[h][_wrjt._pmod].resize(_itv_rows.size());
			_doots[h][_wra._pmod].resize(_itv_rows.size());
		}
	}

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

	inline void heres_my_diffs(vector<float> lsoap[], vector<float> rsoap[])
	{
		_diffs[left_hand] = lsoap;
		_diffs[right_hand] = rsoap;
	}

	inline void operator()(const vector<vector<int>>& itv_rows,
						   const float& rate,
						   vector<float> ldoot[],
						   vector<float> rdoot[])
	{
		// set interval/offset pass specific stuff
		_doots[left_hand] = ldoot;
		_doots[right_hand] = rdoot;
		_itv_rows = itv_rows;
		_rate = rate;

		allocate_doot();
		run_agnostic_pmod_loop();
		run_dependent_pmod_loop();
	}

#pragma region hand agnostic pmod loop
	inline void advance_agnostic_sequencing()
	{
		_fj.advance_sequencing(_mri->ms_now, _mri->notes);
		_tt.advance_sequencing(_mri->ms_now, _mri->notes);
		_tt2.advance_sequencing(_mri->ms_now, _mri->notes);
	}
	inline void setup_agnostic_pmods()
	{
		// these pattern mods operate on all columns, only need basic meta
		// interval data, and do not need any more advanced pattern
		// sequencing
		for (auto& a : _doots) {
			a->resize(_itv_rows.size());

			_fj.setup();
			_tt.setup();
			_tt2.setup();
		}
	}

	inline void set_agnostic_pmods(vector<float> doot[], const int& itv)
	{
		// these pattern mods operate on all columns, only need basic meta
		// interval data, and do not need any more advanced pattern
		// sequencing just set only one hand's values and we'll copy them
		// over (or figure out how not to need to) later
		doot[_s._pmod][itv] = _s(_mitvi);
		doot[_js._pmod][itv] = _js(_mitvi);
		doot[_hs._pmod][itv] = _hs(_mitvi);
		doot[_cj._pmod][itv] = _cj(_mitvi);
		doot[_cjq._pmod][itv] = _cjq(_mitvi);
		doot[_fj._pmod][itv] = _fj();
		doot[_tt._pmod][itv] = _tt();
		doot[_tt2._pmod][itv] = _tt2();
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

		// run twice
		Smooth(doot[_tt._pmod], neutral);
		Smooth(doot[_tt._pmod], neutral);
		Smooth(doot[_tt2._pmod], neutral);
		Smooth(doot[_tt2._pmod], neutral);
	}

	inline void bruh_they_the_same()
	{
		_doots[right_hand][_s._pmod] = _doots[left_hand][_s._pmod];
		_doots[right_hand][_js._pmod] = _doots[left_hand][_js._pmod];
		_doots[right_hand][_hs._pmod] = _doots[left_hand][_hs._pmod];
		_doots[right_hand][_cj._pmod] = _doots[left_hand][_cj._pmod];
		_doots[right_hand][_cjq._pmod] = _doots[left_hand][_cjq._pmod];
		_doots[right_hand][_fj._pmod] = _doots[left_hand][_fj._pmod];
		_doots[right_hand][_tt._pmod] = _doots[left_hand][_tt._pmod];
		_doots[right_hand][_tt2._pmod] = _doots[left_hand][_tt2._pmod];
	}

	inline void run_agnostic_pmod_loop()
	{
		setup_agnostic_pmods();

		// don't use s_init here, we know the first row is always 0.F and
		// therefore the first interval starts at 0.F (unless we do offset
		// passes but that's for later)
		float row_time = 0.F;
		int row_count = 0;
		unsigned row_notes = 0;

		// boop
		for (int itv = 0; itv < _itv_rows.size(); ++itv) {

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
			set_agnostic_pmods(_doots[left_hand], itv);

			// reset any accumulated interval info and set cur index number
			_mitvi.handle_interval_end();
		}

		run_agnostic_smoothing_pass(_doots[left_hand]);

		// copy left -> right for agnostic mods
		bruh_they_the_same();
	}
#pragma endregion

#pragma region hand dependent pmod loop
	// some pattern mod detection builds across rows, see rm_sequencing for
	// an example, actually all sequencing should be done in objects
	// following rm_sequencing's template and be stored in mhi, and then
	// passed to whichever mods need them, but that's for later
	inline void handle_row_dependent_pattern_advancement(const float& row_time)
	{
		_ohj.advance_sequencing(_mhi->_ct, _mhi->_bt);
		_oht.advance_sequencing(_mhi->_mt, _seq._mw_any_ms);
		_rm.advance_sequencing(_mhi->_ct, _mhi->_bt, _mhi->_mt, _seq._as);

		_wrr.advance_sequencing(_mhi->_bt,
								_mhi->_mt,
								_mhi->_last_mt,
								_seq._mw_any_ms.get_now(),
								_seq.get_sc_ms_now(_mhi->_ct));
		_wrjt.advance_sequencing(
		  _mhi->_bt, _mhi->_mt, _mhi->_last_mt, _seq._mw_any_ms);
		_ch.advance_sequencing(_seq._mw_any_ms);
		_roll.advance_sequencing(_mhi->_mt, _seq);
	}

	inline void setup_dependent_mods()
	{
		_oht.setup();
		_roll.setup();
		_rm.setup();
		_wrr.setup();
		_wrjt.setup();
		_wrb.setup();
		_wra.setup();
	}

	inline void set_dependent_pmods(vector<float> doot[], const int& itv)
	{
		doot[_ohj._pmod][itv] = _ohj(_mitvhi);
		doot[_oht._pmod][itv] = _oht(_mitvhi._itvhi);
		doot[_bal._pmod][itv] = _bal(_mitvhi._itvhi);
		doot[_roll._pmod][itv] = _roll(_mitvhi._itvhi, _seq);
		doot[_ch._pmod][itv] = _ch(_mitvhi._itvhi.get_taps_nowi());
		doot[_rm._pmod][itv] = _rm();
		doot[_wrb._pmod][itv] = _wrb(_mitvhi._itvhi);
		doot[_wrr._pmod][itv] = _wrr(_mitvhi._itvhi);
		doot[_wrjt._pmod][itv] = _wrjt(_mitvhi._itvhi);
		doot[_wra._pmod][itv] = _wra(_mitvhi._itvhi, _seq._as);
	}

	inline void run_dependent_smoothing_pass(vector<float> doot[])
	{
		// need to split upohj and cjohj into 2 pmod objects
		Smooth(doot[_ohj._pmod], neutral);
		Smooth(doot[_bal._pmod], neutral);
		// dont do this here, testing internal smooth as advance
		// Smooth(doot[_roll._pmod], neutral);
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
		_wrb.full_reset();
		_wra.full_reset();

		_seq.full_reset();
		_mitvhi.zero();
		_mhi->full_reset();
		_last_mhi->full_reset();
	}

	inline void handle_dependent_interval_end(const int& itv)
	{
		// invoke metaintervalhandinfo interval end FUNCTION
		_mitvhi.interval_end();

		// run pattern mod generation for hand dependent mods
		set_dependent_pmods(_doots[hand], itv);
		_seq.interval_end();
	}

	inline void run_dependent_pmod_loop()
	{
		setup_dependent_mods();

		for (auto& ids : hand_col_ids) {

			float row_time = s_init;
			float last_row_time = s_init;
			float ms_any = ms_init;

			int row_count = 0;
			int last_row_count = 0;
			int last_last_row_count = 0;
			unsigned row_notes = 0U;
			unsigned last_row_notes = 0U;
			unsigned last_last_row_notes = 0U;
			bool last_was_3_note_anch = false;
			col_type ct = col_init;
			CalcMovingWindow<float> teheee;
			full_hand_reset();

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
			vector<float> futurama;
			vector<float> bort;
			float futuramaTEWO = 0.F;
			float barnie = 0.F;
			for (int itv = 0; itv < _itv_rows.size(); ++itv) {
				the_simpsons.clear();
				futurama.clear();
				bort.clear();
				futuramaTEWO = 0.F;
				barnie = 0.F;

				// run the row by row construction for interval info
				for (auto& row : _itv_rows[itv]) {
					row_time = _ni[row].rowTime / _rate;
					row_notes = _ni[row].notes;
					row_count = column_count(row_notes);
					ms_any = ms_from(row_time, last_row_time);



					ct = determine_col_type(row_notes, ids);

					// handle any special cases here before moving on
					if (ct == col_empty) {
						_rm.advance_off_hand_sequencing();
						if (row_count == 2) {
							_rm.advance_off_hand_sequencing();
						}
						continue;
					}

					// test putting generic sequencers here
					// this will keep track of timings from now on
					_seq.advance_sequencing(ct, row_time, ms_any);

					bort.push_back(_seq._as.get_highest_anchor_difficulty());
					barnie =
					  max(barnie, _seq._as.get_highest_anchor_difficulty());

					// mhi will exclusively track pattern configurations
					(*_mhi)(*_last_mhi, ct);

					// update interval aggregation
					_mitvhi._itvhi.set_col_taps(ct);

					handle_row_dependent_pattern_advancement(row_time);

					/* junk in the trunk warning */
					bool is_cj = last_row_count > 1 && row_count > 1;
					bool was_cj = last_row_count > 1 && last_last_row_count > 1;
					bool is_scj = (row_count == 1 && last_row_count > 1) &&
								  ((row_notes & last_row_notes) != 0u);
					bool is_at_least_3_note_anch =
					  ((row_notes & last_row_notes) & last_last_row_notes) !=
					  0u;

					// pushing back ms values, so multiply to nerf
					float pewpew = 3.F;

					if (is_at_least_3_note_anch && last_was_3_note_anch) {
						// biggy boy anchors and beyond
						pewpew = 0.95F;
					} else if (is_at_least_3_note_anch) {
						// big boy anchors
						pewpew = 1.F;
					} else {
						// single note
						if (!is_cj) {
							if (is_scj) {
								// was cj a little bit ago..
								if (was_cj) {
									// single note jack with 2 chords behind it
									pewpew = 1.25F;
								} else {
									// single note, not a jack, 2 chords behind
									// it
									pewpew = 1.5F;
								}
							}
						} else {
							// actual cj
							if (was_cj) {
								// cj now and was cj before, but not necessarily
								// with strong anchors
								pewpew = 1.15F;
							} else {
								// cj now but wasn't even cj before
								pewpew = 1.25F;
							}
						}
					}

					// single note streams / regular jacks should retain the 3x
					// multiplier

					the_simpsons.push_back(
					  max(75.F,
						  min(_seq.get_any_ms_now() * pewpew,
							  _seq.get_sc_ms_now(ct) * pewpew)));

					/* junk in the trunk warning end */
					/* junk in the trunk warning */

					last_last_row_count = row_count;
					last_row_count = row_count;

					last_last_row_notes = last_row_notes;
					last_row_notes = row_notes;

					last_row_time = row_time;
					last_was_3_note_anch = is_at_least_3_note_anch;

					/* junk in the trunk warning end */

					float a = _seq.get_sc_ms_now(ct);
					float b = ms_init;
					if (ct == col_ohjump) {
						b = _seq.get_sc_ms_now(ct, false);
					} else {
						b = _seq.get_cc_ms_now();
					}

					float c = fastsqrt(a) * fastsqrt(b);

					float pineapple = _seq._mw_any_ms.get_cv_of_window(4);
					float porcupine =
					  _seq._mw_sc_ms[col_left].get_cv_of_window(4);
					float sequins =
					  _seq._mw_sc_ms[col_right].get_cv_of_window(4);
					float oioi = 0.5f;
					pineapple = CalcClamp(pineapple + oioi, oioi, 1.F + oioi);
					porcupine = CalcClamp(porcupine + oioi, oioi, 1.F + oioi);
					sequins = CalcClamp(sequins + oioi, oioi, 1.F + oioi);

					float scoliosis = _seq._mw_sc_ms[col_left].get_now();
					float poliosis = _seq._mw_sc_ms[col_right].get_now();
					float obliosis = 0.F;
					if (ct == col_left) {

						obliosis = poliosis / scoliosis;

					} else {

						obliosis = scoliosis / poliosis;
					}
					obliosis = CalcClamp(obliosis, 1.f, 10.F);
					float pewp = cv(std::vector<float>{ scoliosis, poliosis });

					pewp /= obliosis;
					float vertebrae =
					  CalcClamp(mean(std::vector<float>{
								  pineapple, porcupine, sequins }) +
								  pewp,
								oioi,
								1.F + oioi);
					teheee(c / vertebrae);
					futurama.push_back(teheee.get_mean_of_window(2));

					if (_mhi->_bt != base_type_init) {
						++_mitvhi._base_types.at(_mhi->_bt);
						++_mitvhi._meta_types.at(_mhi->_mt);
					}

					futuramaTEWO =
					  max(futuramaTEWO, _rm.get_highest_anchor_difficulty());

					std::swap(_last_mhi, _mhi);
					_mhi->offhand_ohjumps = 0;
					_mhi->offhand_taps = 0;
				}

				handle_dependent_interval_end(itv);
				if (!the_simpsons.empty()) {

					_diffs[hand][CJBase][itv] =
					  CJBaseDifficultySequencing(the_simpsons);

				} else {

					_diffs[hand][CJBase][itv] = 1.F;
				}

				if (!bort.empty()) {

					_diffs[hand][JackBase][itv] = (mean(bort) + barnie) / 2.F;

				} else {

					_diffs[hand][JackBase][itv] = 1.F;
				}

				float berp = 1.F;
				if (!futurama.empty()) {

					berp = TechBaseDifficultySequencing(futurama);
				}

				float scwerp = futuramaTEWO;
				float shlop =
				  weighted_average(berp, _diffs[hand][NPSBase][itv], 5.5F, 9.F);
				_diffs[hand][TechBase][itv] = max(shlop, scwerp);
			}
			run_dependent_smoothing_pass(_doots[hand]);
			DifficultyMSSmooth(_diffs[hand][JackBase]);
			DifficultyMSSmooth(_diffs[hand][CJBase]);

			// ok this is pretty jank LOL, just increment the hand index
			// when we finish left hand
			++hand;
		}
	}
#pragma endregion

	[[nodiscard]] static inline auto make_mod_param_node(
	  const vector<pair<std::string, float*>>& param_map,
	  const std::string& name) -> XNode*
	{
		auto* pmod = new XNode(name);
		for (auto& p : param_map) {
			pmod->AppendChild(p.first, to_string(*p.second));
		}

		return pmod;
	}

	static inline void load_params_for_mod(
	  const XNode* node,
	  const vector<pair<std::string, float*>>& param_map,
	  const std::string& name)
	{
		float boat = 0.F;
		auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (auto& p : param_map) {
			auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}

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

		load_params_for_mod(&params, _s._params, _s.name);
		load_params_for_mod(&params, _js._params, _js.name);
		load_params_for_mod(&params, _hs._params, _hs.name);
		load_params_for_mod(&params, _cj._params, _cj.name);
		load_params_for_mod(&params, _cjq._params, _cjq.name);
		load_params_for_mod(&params, _ohj._params, _ohj.name);
		load_params_for_mod(&params, _bal._params, _bal.name);
		load_params_for_mod(&params, _oht._params, _oht.name);
		load_params_for_mod(&params, _ch._params, _ch.name);
		load_params_for_mod(&params, _rm._params, _rm.name);
		load_params_for_mod(&params, _wrb._params, _wrb.name);
		load_params_for_mod(&params, _wrr._params, _wrr.name);
		load_params_for_mod(&params, _wrjt._params, _wrjt.name);
		load_params_for_mod(&params, _wra._params, _wra.name);
		load_params_for_mod(&params, _fj._params, _fj.name);
		load_params_for_mod(&params, _tt._params, _tt.name);
		load_params_for_mod(&params, _tt2._params, _tt2.name);
	}

	[[nodiscard]] inline auto make_param_node() const -> XNode*
	{
		auto* calcparams = new XNode("CalcParams");
		calcparams->AppendAttr("vers", GetCalcVersion());

		calcparams->AppendChild(make_mod_param_node(_s._params, _s.name));
		calcparams->AppendChild(make_mod_param_node(_js._params, _js.name));
		calcparams->AppendChild(make_mod_param_node(_hs._params, _hs.name));
		calcparams->AppendChild(make_mod_param_node(_cj._params, _cj.name));
		calcparams->AppendChild(make_mod_param_node(_cjq._params, _cjq.name));
		calcparams->AppendChild(make_mod_param_node(_ohj._params, _ohj.name));
		calcparams->AppendChild(make_mod_param_node(_bal._params, _bal.name));
		calcparams->AppendChild(make_mod_param_node(_oht._params, _oht.name));
		calcparams->AppendChild(make_mod_param_node(_ch._params, _ch.name));
		calcparams->AppendChild(make_mod_param_node(_rm._params, _rm.name));
		calcparams->AppendChild(make_mod_param_node(_wrb._params, _wrb.name));
		calcparams->AppendChild(make_mod_param_node(_wrr._params, _wrr.name));
		calcparams->AppendChild(make_mod_param_node(_wrjt._params, _wrjt.name));
		calcparams->AppendChild(make_mod_param_node(_wra._params, _wra.name));
		calcparams->AppendChild(make_mod_param_node(_fj._params, _fj.name));
		calcparams->AppendChild(make_mod_param_node(_tt._params, _tt.name));
		calcparams->AppendChild(make_mod_param_node(_tt2._params, _tt2.name));

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
