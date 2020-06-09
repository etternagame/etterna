#pragma once
#include "Etterna/Globals/MinaCalc/Dependent/HD_MetaSequencing.h"

enum rm_behavior
{
	rmb_off_tap_oh,
	rmb_off_tap_sh,
	rmb_anchor,
	rmb_jack,
	rmb_init,
};

enum rm_status
{
	rm_inactive,
	rm_running,
};

struct RunningMan
{
	// all taps contained in this runningman sequence
	int ran_taps = 0;

	// total length of the anchor
	int _len = 0;

	// one hand trill taps
	int oht_taps = 0;
	// current oht sequence length
	int oht_len = 0;

	// any off anchor taps
	int off_taps = 0;
	int off_len = 0;

	// off anchor taps on the same hand, i.e. the 2s in 1211121
	int off_taps_sh = 0;

	// jack taps (like, actual jacks in the runningman)
	int jack_taps = 0;
	int jack_len = 0;

	// consecutive anchors sequence length, track this to throw out 2h trills
	int anch_len = 0;

	inline void full_reset()
	{
		// don't touch anchor col
		_len = 0;

		off_taps_sh = 0;
		oht_taps = 0;
		oht_len = 0;
		off_taps = 0;
		off_len = 0;

		jack_taps = 0;
		jack_len = 0;

		anch_len = 0;
	}

	inline void add_off_tap_sh()
	{
		++off_taps_sh;
		add_off_tap();
	}

	inline void add_off_tap()
	{
		++off_len;
		++off_taps;
		++ran_taps;
	}

	inline void add_anchor_tap()
	{
		++anch_len;
		++ran_taps;
	}

	inline void add_jack_tap()
	{
		++jack_len;
		++jack_taps;
		++ran_taps;
	}
	
	inline void end_jack_and_anch_runs()
	{
		end_anch_run();
		end_jack_run();
	}
	inline void end_anch_run() { anch_len = 0; }
	inline void end_jack_run() { jack_len = 0; }
	inline void end_off_tap_run() { off_len = 0; }

	inline void restart()
	{
		/* we will probably be resetting much more than we are restarting, so to
		 * reduce computational expense, only set any values back to 0 while
		 * sequencing after a restart, and remove the old restart function and
		 * replace it with an inactive status. this is the old reset block,
		 * minus _len, ran_taps, and time, those are set above because they are
		 * already known */

		off_taps_sh = 0;
		off_taps = 0;
		off_len = 0;

		oht_taps = 0;
		oht_len = 0;

		jack_taps = 0;
		jack_len = 0;

		anch_len = 0;
	}
};

// used by ranmen mod, for ranmen sequencing (doesn't have a sequence struct and
// probably should?? this should just be logic only)
struct RM_Sequencer
{
	// params.. loaded by runningman and then set from there
	int max_oht_len = 0;
	int max_off_spacing = 0;
	int max_burst_len = 0;
	int max_jack_len = 0;

	// end with same hand off anchor taps, this is so 2h trills don't get
	// flagged as runningmen
	int max_anchor_len = 0;

	inline void set_params(const float& moht,
						   const float& moff,
						   const float& mburst,
						   const float& mjack,
						   const float& manch)
	{
		max_oht_len = static_cast<int>(moht);
		max_off_spacing = static_cast<int>(moff);
		max_burst_len = static_cast<int>(mburst);
		max_jack_len = static_cast<int>(mjack);
		max_anchor_len = static_cast<int>(manch);
	}

	col_type _ct = col_init;
	rm_status _status = rm_inactive;
	rm_behavior _rmb = rmb_init;
	rm_behavior _last_rmb = rmb_init;

	RunningMan _rm;

	// try to allow 1 burst?
	bool is_bursting = false;
	bool had_burst = false;

	float _max_ms = ms_init;
	float last_anchor_time = s_init;

	float _start = s_init;

#pragma region functions

	// reset
	inline void reset() {}

	inline void full_reset()
	{
		// don't touch anchor col
		_start = s_init;

		is_bursting = false;
		had_burst = false;

		_rm.full_reset();
	}

	/* restart only if we have just reset and there is a valid last rm_behavior
	 * to start from. this is so we don't restart a runningman sequence
	 * preceeded by pure jacks, (though there is some question about allowing
	 * for only a single jack to start a new sequence, and how to handle doing
	 * so). since restarting means we already have an anchor length of 2 (see
	 * anchor sequencer) we can check for offtaps same hand, or offhand taps as
	 * the last behavior to allow restarting. for the moment only
	 * offtaps_samehand will be allowed to restart */
	inline void restart(const Anchor_Sequencing& as)
	{
		/* we are restarting the runningman sequence, this means we know there
		 * is something valid between now and the last note in this column to do
		 * so, that means the _runningman_ start time might be different from
		 * whatever is generally going on anchor_wise, we don't care about that,
		 * so set _start from as's _last */
		_start = as._last;
		_rm._len = as._len;

		// technically if we are restarting we know we have 3 taps, but let
		// last_rmb handling take care of the third for clarity
		_rm.ran_taps += 2;

		/* we will probably be resetting much more than we are restarting, so to
		 * reduce computational expense, only set any values back to 0 while
		 * sequencing after a restart, and remove the old restart function and
		 * replace it with an inactive status. this is the old reset block,
		 * minus _len, ran_taps, and time, those are set above because they are
		 * already known */

		{
			is_bursting = false;
			had_burst = false;
			_rm.restart();
		}

		// retroactively handle whatever behavior allowed the restart
		handle_last_rmb();
	}

	inline auto should_restart() -> bool { return _last_rmb == rmb_off_tap_sh; }

	inline void end_off_tap_run()
	{
		// allow only 1 burst
		if (is_bursting) {
			is_bursting = false;
			had_burst = true;
		}
		// reset off_len counter
		_rm.end_off_tap_run();
	}

	// optimization for restarting that skips max len checks
	inline void handle_last_rmb()
	{
		// only viable restart mechanism for now
		switch (_last_rmb) {
			case rmb_off_tap_sh:
				_rm.add_off_tap_sh();
				break;
			default:
				break;
		}
	}

	inline auto off_len_exceeds_max() -> bool
	{
		// haven't exceeded anything
		if (_rm.off_len <= max_off_spacing)
			return false;

		if (had_burst) {
			// already had a burst and exceeding normal limit
			return true;
		} else if (_rm.off_len > max_burst_len) {
			// exceeded the burst limit
			return true;
		} else {
			// have exceeded the normal limit but not had a burst yet, set
			// bursting to true and return false
			is_bursting = true;
			return false;
		}
	}

	inline auto jack_len_exceeds_max() -> bool
	{
		return _rm.jack_len > max_jack_len;
	}

	inline auto anch_len_exceeds_max() -> bool
	{
		return _rm.anch_len > max_anchor_len;
	}

	inline auto oht_len_exceeds_max() -> bool
	{
		return _rm.oht_len > max_anchor_len;
	}

	// executed if incoming ct == _ct
	inline void handle_anchor_behavior(const Anchor_Sequencing& as)
	{
		// handle anchor logic here

		// too long since we saw an off anchor same hand tap.. it's probably a
		// trill or something
		if (anch_len_exceeds_max()) {
			_status = rm_inactive;
			return;
		}

		// make sure anchor sequence col is our anchor col
		assert(as._ct == _ct);
		switch (as._status) {
			case reset_too_slow:
			case reset_too_fast:

				// anchor has changed speeds to a significant degree,
				// restart if we are able to, otherwise flag the runningman
				// as inactive
				if (should_restart()) {
					restart(as);
				} else {
					_status = rm_inactive;
				}

				break;
			case anchoring:

				_rm.add_anchor_tap();
				_rm.end_off_tap_run();

				return;
			case anch_init:
				// do nothing
				break;
		}
	}

	inline void handle_off_tap_sh_behavior()
	{
		// add before running length checks
		_rm.add_off_tap_sh();
		if (off_len_exceeds_max()) {
			// don't reset anything, just flag as inactive
			_status = rm_inactive;
		} else {
			// we have an offanchor tap on the same hand, end any jack or
			// consecutive anchor sequences
			_rm.end_jack_and_anch_runs();
		}
	}

	inline void handle_off_tap_oh_behavior()
	{
		_rm.add_off_tap();
		if (off_len_exceeds_max()) {
			_status = rm_inactive;
		} else {
			// we have an offanchor tap on the other hand, end any jack run, but
			// not an anchor run, those should only be broken by same hand taps
			_rm.end_jack_run();
		}
	}

	inline void handle_jack_behavior()
	{
		_rm.add_jack_tap();
		if (jack_len_exceeds_max()) {
			_status = rm_inactive;
		} else {
			end_off_tap_run();
		}
	}

	/* oht's are a subtype of off_tap_sh, and the behavior will just fall
	 * through to the latter's, so don't do anything outside of oht values
	 * or reset anything, it'll be redundant at best and bug prone at worst
	 */
	inline void handle_oht_behavior(const col_type& ct)
	{
		/* to be explicit about the goal here, given 111212111 any reasonable
		 * player would conclude there was a 4 note oht in the middle of a
		 * runningman, and while rare (because it's hard as shit) it's
		 * acceptable, the same thing applies to a 6 note oht inside a
		 * runningman, though those are even rarer, however what we care about
		 * is the threshold at which this can be jumpjacked, which is about 8
		 * oht taps total, dependent on speed (this is already pretty generous
		 * and i don't want to handle diffrential speeds here). now since a 7
		 * note ohtrill in the context of a runningman is meaningless (think
		 * about it) we only really care about the number of consecutive
		 * off_taps_sh */
		if (ct != _ct) {
			// metatype won't be set until it finds 1212, but we want to
			// explicitly track the number of off_anchor taps in the oht, so
			// boost by 1 when we see meta_oht and oht_len == 0
			if (_rm.oht_len == 0)
				++_rm.oht_len;

			// ok i got lazy/impatient here
			++_rm.oht_len;
			++_rm.oht_taps;
			if (oht_len_exceeds_max()) {
				_status = rm_inactive;
			}
		}
	}

	inline void handle_rmb(const Anchor_Sequencing& as)
	{
		assert(_status == rm_running);
		switch (_rmb) {
			case rmb_off_tap_oh:
				// should only ever be called from advance_off_hand_sequencing
				assert(0);
				break;
			case rmb_off_tap_sh:
				handle_off_tap_sh_behavior();
				break;
			case rmb_anchor:
				handle_anchor_behavior(as);
				break;
			case rmb_jack:
				handle_jack_behavior();
				break;
			case rmb_init:
				break;
			default:
				break;
		}
	}

	/* rm_sequencing is the only hand dependent mod atm that actually cares
	 * about basic off_hand information, so this should be called to update
	 * using that information before the ct == col_empty continue block in ulbu.
	 */
	inline void advance_off_hand_sequencing()
	{
		handle_off_tap_oh_behavior();
		_last_rmb == rmb_off_tap_oh;
	}

	inline void operator()(const col_type& ct,
						   const base_type& bt,
						   const meta_type& mt,
						   const Anchor_Sequencing& as)
	{
		// no matter how you slice it you can't have a runningman with 2 taps
		if (bt == base_type_init)
			return;

		// cosmic brain handling of ohts, this won't interfere with the
		// determinations in the behavior block, but it can set rm_inactive (as
		// it should be able to)
		if (mt == meta_cccccc) {
			handle_oht_behavior(ct);
		}

		// determine what we should do
		switch (bt) {
			case base_left_right:
			case base_right_left:
				if (_ct == ct) {
					// this is an anchor
					_rmb = rmb_anchor;
				} else {
					// this is a same hand off anchor tap
					_rmb = rmb_off_tap_sh;
				}
				break;
			case base_jump_single:
				if (_last_rmb == rmb_off_tap_oh) {
					// if we have a jump -> single, and the last note was an
					// offhand tap, and the single is the anchor col, then
					// we have an anchor
					if (_ct == ct) {
						_rmb = rmb_anchor;
					} else {
						// this is a same hand off anchor tap
						_rmb = rmb_off_tap_sh;
					}
				} else {
					// if we are jump -> single and the last note was _not_
					// an offhand hand tap, we have a jack
					_rmb = rmb_jack;
				}
				break;
			case base_single_single:
			case base_single_jump:
				// if last note was an offhand tap, this is by
				// definition part of the anchor
				if (_last_rmb == rmb_off_tap_oh) {
					_rmb = rmb_anchor;
				} else {
					// if not, a jack
					_rmb = rmb_jack;
				}
				break;
			case base_jump_jump:
				/* this is kind of a gray area, given that the difficulty of
				 * runningmen comes from the tight turns on the same hand...
				 * we will treat this as a jack even though technically it's
				 * an "anchor" when the last tap was an offhand tap */
				_rmb = rmb_jack;
				break;
			case base_type_init:
				break;
			default:
				assert(0);
				break;
		}

		// do thing, maybe

		/* only allow same hand off taps after an anchor to begin a runningman
		 * sequence for now, this means we are rm_inactive, we are currently
		 * using behavior _rmb_off_tap_sh, and _last_rmb was rmb_anchor */
		if (_status == rm_inactive) {
			if (_rmb == rmb_off_tap_sh && _last_rmb == rmb_anchor) {

				// ok we can start
				_status == rm_running;

				// apply restart behavior since it's exactly what we want (i
				// know the naming is a bit confusing, restarting should only
				// apply to resets that occur after already starting but w.e)
				restart(as);
			}
		} else {
			// if we're not inactive, just do normal behavior
			handle_rmb(as);
		}

		// remember what we did last
		_last_rmb = _rmb;
	}

	inline auto get_difficulty() -> float
	{
		if (_status == rm_inactive)
			return 1.f;
		float flool = ms_from(last_anchor_time, _start);

		// may be unnecessary
		float glunk =
		  CalcClamp(static_cast<float>(_rm.off_taps_sh) / 2.f, 0.1f, 1.f);

		float pule = (flool + 9.F) / static_cast<float>(_rm._len);
		float drool = ms_to_scaled_nps(pule);
		return drool * glunk;
	}
};
