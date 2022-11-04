#pragma once
#include "../HD_MetaSequencing.h"
#include "../../UlbuAcolytes.h"

static const float rma_diff_scaler = 1.06F * basescalers[Skill_Technical];

enum rm_behavior
{
	rmb_off_tap_oh,
	rmb_off_tap_sh,
	rmb_anchor,
	rmb_jack, // only for the anchor col, not any col
	rmb_init,
};

enum rm_status
{
	rm_inactive,
	rm_running,
};

/// Maintains the information pertaining to a single runningman.
/// This runningman is for one finger, so there are 4 of them.
/// The finger this is built for is the anchor finger.
struct RunningMan
{
	/// all taps contained in this runningman sequence
	int ran_taps = 0;

	/// total length of the anchor
	int _len = 0;

	/// any off anchor taps
	int off_taps = 0;
	int off_len = 0;

	/// off anchor taps on the same hand, i.e. the 2s in 1211121
	int off_taps_sh = 0;

	/// one hand trill taps
	int oht_taps = 0;
	/// current oht sequence length
	int oht_len = 0;

	/// it's not really a runningman if the anchor is on the off column is it
	int ot_sh_len = 0;

	/// jack taps (like, actual jacks in the runningman)
	int jack_taps = 0;
	int jack_len = 0;

	/// consecutive anchors sequence length, track this to throw out 2h trills
	int anch_len = 0;

	void full_reset()
	{
		// don't touch anchor col
		_len = 0;

		off_taps_sh = 0;

		off_taps = 0;
		off_len = 0;

		oht_taps = 0;
		oht_len = 0;

		jack_taps = 0;
		jack_len = 0;

		anch_len = 0;
	}

	void add_off_tap_sh()
	{
		++off_taps_sh;
		++ot_sh_len;
		add_off_tap();
	}

	void add_off_tap()
	{
		++off_len;
		++off_taps;
		++ran_taps;
	}

	void add_oht_tap()
	{
		++oht_len;
		++oht_taps;
	}

	void add_anchor_tap()
	{
		++_len;
		++anch_len;
		++ran_taps;
	}

	void add_jack_tap()
	{
		++jack_len;
		++jack_taps;
		++ran_taps;
	}

	void end_jack_and_anch_runs()
	{
		end_anch_run();
		end_jack_run();
	}

	void end_anch_run() { anch_len = 0; }
	void end_jack_run() { jack_len = 0; }

	void end_off_tap_run()
	{
		off_len = 0;
		ot_sh_len = 0;
	}

	void restart()
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

	/// any off taps to anchor len
	[[nodiscard]] auto get_off_tap_prop() const -> float
	{
		if (off_taps == 0)
			return 0.F;

		return (static_cast<float>(_len) / (static_cast<float>(off_taps)));
	}

	/// off hand taps to anchor len
	[[nodiscard]] auto get_offhand_tap_prop() const -> float
	{
		if (off_taps - off_taps_sh <= 0)
			return 0.F;

		return ((static_cast<float>(off_taps - off_taps_sh)) /
				static_cast<float>(_len));
	}

	/// same hand taps to anchor len
	[[nodiscard]] auto get_off_tap_same_prop() const -> float
	{
		if (off_taps_sh == 0)
			return 0.F;

		return ((static_cast<float>(off_taps_sh)) / static_cast<float>(_len));
	}
};

/// used by ranmen mod, for ranmen sequencing (doesn't have a sequence struct and
/// probably should?? this should just be logic only)
struct RM_Sequencer
{
	// params.. loaded by runningman and then set from there
	int max_oht_len = 0;
	int max_off_len = 0;
	int max_ot_sh_len = 0;
	int max_burst_len = 0;
	int max_jack_len = 0;

	// end with same hand off anchor taps, this is so 2h trills don't get
	// flagged as runningmen
	int max_anchor_len = 0;

	void set_params(const float& moht,
					const float& moff,
					const float& motsh,
					const float& mburst,
					const float& mjack,
					const float& manch)
	{
		max_oht_len = static_cast<int>(moht);
		max_off_len = static_cast<int>(moff);
		max_ot_sh_len = static_cast<int>(motsh);
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

	float last_anchor_time = s_init;
	float _start = s_init;

#pragma region functions

	void full_reset()
	{
		// don't touch anchor col

		_status = rm_inactive;
		_rmb = rmb_init;
		_last_rmb = rmb_init;

		_start = s_init;
		last_anchor_time = s_init;

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
	void restart(const Anchor_Sequencing& as)
	{
		assert(_last_rmb == rmb_off_tap_sh);

		/* we are restarting the runningman sequence because the anchor sequence
		 * has reset and our last update was a same hand off tap, or because we
		 * were inactive, had a same hand off tap last update, and are now on
		 * the anchor col. as._last is the last seen col of the anchor, since we
		 * can only restart when we're on an anchor col, the anchor col should
		 * have already been updated and _last would be now, so calculate the
		 * start time from as._sc_ms */
		_start = as._last - (as._sc_ms / 1000.F);

		// should always be equivalent to NOW,
		last_anchor_time = as._last;

		assert(_start < last_anchor_time);
		_rm._len = 2;

		// technically if we are restarting we know we have 3 taps, but let
		// last_rmb handling take care of the third for clarity
		_rm.ran_taps = 2;

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

	auto should_restart() const -> bool { return _last_rmb == rmb_off_tap_sh; }

	void end_off_tap_run()
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
	void handle_last_rmb()
	{
		// only viable start/restart mechanisms for now
		switch (_last_rmb) {
			// ok this is jank af but if rmb_anchor is _last_rmb when restarting
			// it means we've started up again from the initial start logic, and
			// so we are _on_ rmb_off_tap_sh, so let this fall through
			case rmb_off_tap_sh:
				_rm.add_off_tap_sh();
				break;
			default:
				assert(0);
				break;
		}
	}

	auto off_len_exceeds_max() -> bool
	{
		// haven't exceeded anything
		if (_rm.off_len <= max_off_len) {
			return false;
		}

		// already had a burst and exceeding normal limit or exceeded the burst
		// limit
		if (had_burst || _rm.off_len > max_burst_len) {
			return true;
		}

		// have exceeded the normal limit but not had a burst yet, set
		// bursting to true and return false
		is_bursting = true;
		return false;
	}

	auto ot_sh_len_exceeds_max() const -> bool
	{
		return _rm.ot_sh_len > max_ot_sh_len;
	}

	auto jack_len_exceeds_max() const -> bool
	{
		return _rm.jack_len > max_jack_len;
	}

	auto anch_len_exceeds_max() const -> bool
	{
		return _rm.anch_len > max_anchor_len;
	}

	auto oht_len_exceeds_max() const -> bool
	{
		return _rm.oht_len > max_oht_len;
	}

	// executed if incoming ct == _ct
	void handle_anchor_behavior(const Anchor_Sequencing& as)
	{
		// handle anchor logic here

		// too long since we saw an off anchor same hand tap.. it's probably a
		// trill or something
		if (anch_len_exceeds_max()) {
			_status = rm_inactive;
			return;
		}

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
			case anch_init:
				// do nothing
				break;
		}
	}

	void handle_off_tap_sh_behavior()
	{
		// add before running length checks
		_rm.add_off_tap_sh();
		if (off_len_exceeds_max() || ot_sh_len_exceeds_max()) {
			// don't reset anything, just flag as inactive
			_status = rm_inactive;
		} else {
			// we have an offanchor tap on the same hand, end any jack or
			// consecutive anchor sequences
			_rm.end_jack_and_anch_runs();
		}
	}

	void handle_off_tap_oh_behavior()
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

	void handle_jack_behavior()
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
	void handle_oht_behavior(const col_type& ct)
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
			/* metatype won't be set until it finds 1212, but we want to
			 * explicitly track the number of off_anchor taps in the oht, so
			 * boost by 1 when we see meta_oht and oht_len == 0 */
			if (_rm.oht_len == 0) {
				_rm.add_oht_tap();
			}

			_rm.add_oht_tap();
			if (oht_len_exceeds_max()) {
				_status = rm_inactive;
			}
		}
	}

	void handle_rmb(const Anchor_Sequencing& as)
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
			default:
				break;
		}
	}

	/* rm_sequencing is the only hand dependent mod atm that actually cares
	 * about basic off_hand information, so this should be called to update
	 * using that information before the ct == col_empty continue block in ulbu.
	 */
	void advance_off_hand_sequencing()
	{
		handle_off_tap_oh_behavior();
		_last_rmb = rmb_off_tap_oh;
	}

	void operator()(const col_type& ct,
					const base_type& bt,
					const meta_type& mt,
					const Anchor_Sequencing& as)
	{
		/* cosmic brain handling of ohts, this won't interfere with the
		 * determinations in the behavior block, but it can set rm_inactive (as
		 * it should be able to) */
		if (mt == meta_cccccc) {
			handle_oht_behavior(ct);
		}

		/* update our last anchor time , we don't handle this in the anchoring
		 * block because technically jacks can either be on the anchor column or
		 * not, and i don't want to have to split logic again between anchor
		 * column jacks and off anchor jacks on the same hand */
		last_anchor_time = as._last;

		// if anchor_sequencing passes forward s_init, reset everything
		// this means the rm for this finger should definitely be dead
		// (nuclear bandaid, probably just want to set status to inactive)
		if (last_anchor_time == s_init) {
			full_reset();
			return;
		}

		// determine what we should do
		switch (bt) {
			case base_left_right:
			case base_right_left:
			case base_single_single:
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
			case base_single_jump:
			case base_jump_jump:
				// if last note was an offhand tap, this is by
				// definition part of the anchor
				if (_last_rmb == rmb_off_tap_oh) {
					_rmb = rmb_anchor;
				} else {
					// if not, a jack
					_rmb = rmb_jack;
				}
				break;
			case base_type_init:
				// bail and don't set anything
				return;
			default:
				assert(0);
				break;
		}

		/* only allow same hand off taps after an anchor to begin a runningman
		 * sequence for now, this means we are rm_inactive, we are currently
		 * using behavior _rmb_off_tap_sh, and _last_rmb was rmb_anchor */
		if (_status == rm_inactive) {
			if (_rmb == rmb_anchor && _last_rmb == rmb_off_tap_sh) {

				// ok we can start
				_status = rm_running;
				restart(as);
			}
		} else {
			// if we're not inactive, just do normal behavior
			handle_rmb(as);
		}

		// remember what we did last
		_last_rmb = _rmb;
	}

	[[nodiscard]] auto get_difficulty() const -> float
	{
		if (_status == rm_inactive || _rm._len < 3) {
			return 1.F;
		}

		const auto flool = ms_from(last_anchor_time, _start);

		const auto len = static_cast<float>(_rm._len);
		const auto len_1 = static_cast<float>(_rm._len - 1);

		const auto pule = (flool / len_1) * (len / len_1);
		const auto drool = ms_to_scaled_nps(pule) * rma_diff_scaler;
		return drool;
	}
};
