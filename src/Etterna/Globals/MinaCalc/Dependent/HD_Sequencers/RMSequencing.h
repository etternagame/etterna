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
	rm_status _statuc = rm_inactive;
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

	// sequencing counters
	// only allow this rm's anchor col to start sequences
	bool in_the_nineties = false;
	// try to allow 1 burst?
	bool is_bursting = false;
	bool had_burst = false;

	int ran_taps = 0;
	int _len = 0;

	// off anchor taps on the same hand, i.e. the 2s in 1211121
	int off_taps_sh = 0;
	int oht_taps = 0;
	int oht_len = 0;
	int off_taps = 0;
	int off_len = 0;

	int jack_taps = 0;
	int jack_len = 0;

	int anch_len = 0;

	float _max_ms = ms_init;
	float _sc_ms = 0.F;
	float last_anchor_time = s_init;

	float _start = s_init;

#pragma region functions

	// reset
	inline void reset() {}

	inline void full_reset()
	{
		// don't touch anchor col
		_len = 0;
		_start = s_init;

		in_the_nineties = false;
		is_bursting = false;
		had_burst = false;

		ran_taps = 0;

		off_taps_sh = 0;
		oht_taps = 0;
		oht_len = 0;
		off_taps = 0;
		off_len = 0;

		jack_taps = 0;
		jack_len = 0;
	}

	inline void handle_off_tap()
	{
		if (!in_the_nineties) {
			return;
		}

		++ran_taps;
		++off_taps;
		++off_len;

		// offnote, reset jack length & oht length
		jack_len = 0;

		// handle progression for increasing off_len
		handle_off_tap_progression();
	}

	inline void handle_off_tap_progression()
	{
		// resume off tap progression caused by another consecutive off tap
		// normal behavior if we have already allowed for 1 burst, reset if the
		// offtap sequence exceeds the spacing limit; this will also catch
		// bursts that exceed the max burst length
		if (had_burst) {
			if (off_len > max_off_spacing) {
				reset();
				return;
			}
			// don't care about any other behavior here
			return;
		}

		// if we are in a burst, allow it to finish and when it does set the
		// had_burst flag rather than resetting, if the burst continues beyond
		// the max burst length then it will be reset via other means
		// (we must be in a burst if off_len == max_burst_len)
		if (off_len == max_burst_len) {
			handle_off_tap_completion();
			return;
		}

		// haven't had or started a burst yet, if we exceed max_off_spacing, set
		// is_bursting to true and allow it to continue, otherwise, do nothing
		if (off_len > max_off_spacing) {
			is_bursting = true;
		}
	}

	inline void handle_anchor_progression()
	{
		// start a sequence whenever we hit this rm's anchor col, if we aren't
		// already in one
		if (in_the_nineties) {
			// break the anchor if the next note is too much slower than the
			// lowest one, but only after we've already set the initial anchor
			_sc_ms = ms_from(now, last_anchor_time);
			// account for float precision error and small bpm flux
			if (_sc_ms > _max_ms + anchor_buffer_ms) {
				reset();
			} else if (_sc_ms * 2.5F < _max_ms) {

				reset();
				_start = last_anchor_time;
				_max_ms = _sc_ms;
				_len = 2;
			} else {
				// increase anchor length and set new cutoff point
				_max_ms = _sc_ms;
			}
		} else {
			// set first anchor val
			_max_ms = 5000.F;
			_start = last_anchor_time;
			in_the_nineties = true;
		}

		last_anchor_time = now;
		++ran_taps;
		++_len;

		// handle completion of off tap progression
		handle_off_tap_completion();
	}

	inline void handle_jack_progression()
	{
		++ran_taps;
		//++_len; // do this for jacks?
		++jack_len;
		++jack_taps;

		// handle completion of off tap progression
		handle_off_tap_completion();

		// make sure to set the anchor col when resetting if we exceed max jack
		// len
		if (jack_len > max_jack_len) {
			reset();
		}
	}

	inline void handle_cross_column_branching()
	{

		if (_ct != now_col && in_the_nineties) {
			handle_off_tap();
			// same hand offtap
			++off_taps_sh;
			return;
		}
		handle_anchor_progression();
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
		_len = as._len;

		// technically if we are restarting we know we have 3 taps, but let
		// last_rmb handling take care of the third for clarity
		ran_taps += 2;

		/* we will probably be resetting much more than we are restarting, so to
		 * reduce computational expense, only set any values back to 0 while
		 * sequencing after a restart, and remove the old restart function and
		 * replace it with an inactive status. this is the old reset block,
		 * minus _len, ran_taps, and time, those are set above because they are
		 * already known */

		{
			in_the_nineties = false;
			is_bursting = false;
			had_burst = false;

			off_taps_sh = 0;
			off_taps = 0;
			off_len = 0;

			oht_taps = 0;
			oht_len = 0;

			jack_taps = 0;
			jack_len = 0;
		}

		// retroactively handle whatever behavior allowed the restart
		handle_last_rmb();
	}

	inline auto should_restart() -> bool { return _last_rmb == rmb_off_tap_sh; }

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

	inline void end_off_tap_run()
	{
		// allow only 1 burst
		if (is_bursting) {
			is_bursting = false;
			had_burst = true;
		}
		// reset off_len counter
		off_len = 0;
	}

	inline void end_jack_and_anch_runs()
	{
		end_anch_run();
		end_jack_run();
	}
	inline void end_anch_run() { anch_len = 0; }
	inline void end_jack_run() { jack_len = 0; }

	// optimization for restarting that skips max len checks
	inline void handle_last_rmb()
	{
		// only viable restart mechanism for now
		switch (_last_rmb) {
			case rmb_off_tap_sh:
				add_off_tap_sh();
				break;
			default:
				break;
		}
	}

	inline auto off_len_exceeds_max() -> bool
	{
		// haven't exceeded anything
		if (off_len <= max_off_spacing)
			return false;

		if (had_burst) {
			// already had a burst and exceeding normal limit
			return true;
		} else if (off_len > max_burst_len) {
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
		return jack_len > max_jack_len;
	}

	inline auto anch_len_exceeds_max() -> bool
	{
		return anch_len > max_anchor_len;
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

				add_anchor_tap();
				end_off_tap_run();

				return;
			case anch_init:
				// do nothing
				break;
		}
	}

	inline void handle_off_tap_sh_behavior()
	{
		// add before running length checks
		add_off_tap_sh();
		if (off_len_exceeds_max()) {
			// don't reset anything, just flag as inactive
			_status = rm_inactive;
		} else {
			// we have an offanchor tap on the same hand, end any jack or
			// consecutive anchor sequences
			end_jack_and_anch_runs();
		}
	}

	inline void handle_off_tap_oh_behavior()
	{
		add_off_tap();
		if (off_len_exceeds_max()) {
			_status = rm_inactive;
		} else {
			// we have an offanchor tap on the other hand, end any jack run, but
			// not an anchor run, those should only be broken by same hand taps
			end_jack_run();
		}
	}

	inline void handle_jack_behavior()
	{
		add_jack_tap();
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

			if (oht_len == 0)
				++oht_len;
			++oht_taps;
			if (oht_len > max_oht_len) {
				_status = rm_inactive;
			}
		}
	}

	inline void handle_rmb(const Anchor_Sequencing& as)
	{
		assert(_status == rm_running);
		switch (_rmb) {
			case rmb_off_tap_oh:
				handle_off_tap_oh_behavior();
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

	inline void operator()(const col_type& ct,
						   const base_type& bt,
						   const meta_type& mt,
						   const float& row_time,
						   const int& offhand_taps,
						   const Anchor_Sequencing& as)
	{
		// no matter how you slice it you can't have a runningman with 2 taps
		if (bt == base_type_init)
			return;

		// THIS IS IMPOSSIBLE BECAUSE RANMEN MOD IS NEVER CALLED ON EMPTY ROWS
		// FOR A GIVEN HAND, this is here to remind myself that the way that i'm
		// handling fully off hand taps is garbage and should be a lot better
		// and maybe ranmen _should_ be run independent of the col empty loop
		// if (ct == col_empty)
		// 	return;

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
				if (offhand_taps > 0) {
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
				if (offhand_taps > 0) {
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
	}

	// play catch up, treat offhand jumps like 2 offtaps
	if (in_the_nineties && offhand_taps > 0) {
		// reset oht len if we hit this (not really robust buuuut)
		oht_len = 0;

		for (int i = 0; i < offhand_taps; ++i) {
			handle_off_tap();
		}
	}

	// cosmic brain
	if (mt == meta_cccccc) {
		handle_oht_progression();
	}

	switch (bt) {
		case base_left_right:
		case base_right_left:
			handle_cross_column_branching();
			break;
		case base_jump_single:
			if (offhand_taps > 0) {
				// if we have a jump -> single, and the last
				// note was an offhand tap, and the single
				// is the anchor col, then we have an anchor
				if ((now_col == col_left && _ct == col_left) ||
					(now_col == col_right && _ct == col_right)) {
					handle_anchor_progression();
				} else {
					// otherwise we have an off anchor tap
					handle_off_tap();
					// same hand offtap
					++off_taps_sh;
				}
			} else {
				// if we are jump -> single and the last
				// note was not an offhand hand tap, we have
				// a jack
				handle_jack_progression();
			}
			break;
		case base_single_single:
		case base_single_jump:
			// if last note was an offhand tap, this is by
			// definition part of the anchor
			if (offhand_taps > 0) {
				handle_anchor_progression();
			} else {
				// if not, a jack
				handle_jack_progression();
			}
			break;
		case base_jump_jump:
			// this is kind of a gray area, given that
			// the difficulty of runningmen comes from
			// the tight turns on the same hand... we
			// will treat this as a jack even though
			// technically it's an "anchor" when the
			// last tap was an offhand tap
			handle_jack_progression();
			break;
		case base_type_init:
			if (now_col == _ct) {
				handle_anchor_progression();
			}
			break;
		default:
			assert(0);
			break;
	}
}

// copied from generic sequencer, rm_sequencing should just utilize the
// generic anchor sequencer, but cba now
inline auto
get_difficulty() -> float
{
	if (!in_the_nineties)
		return 1.f;
	float flool = ms_from(last_anchor_time, _start);
	float glunk = CalcClamp(static_cast<float>(off_taps_sh) / 2.f, 0.1f, 1.f);
	float pule = (flool + 9.F) / static_cast<float>(_len);
	float drool = ms_to_scaled_nps(pule);
	return drool * glunk;
}
}
;
