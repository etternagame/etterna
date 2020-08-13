#pragma once
#include <array>
#include <cassert>

static const int max_flam_jammies = 4;
/* keep track of potential flam formation */
struct flam
{
	// cols seen
	unsigned unsigned_unseen = 0U;

	// size in ROWS, not columns, if flam size == 1 we do not yet have a flam
	// and we have no current relevant values in ms[], any that are set will be
	// leftovers from the last sequence, this is to optimize out setting
	// rowtimes or calculating ms times
	int size = 1;

	// size > 1, is this actually more efficient than calling a bool check func?
	bool flammin = false;

	// ms values, 3 ms values = 4 rows, optimize by just recycling values
	// without resetting and indexing up to the size counter to get duration
	std::array<float, 3> ms = {
		0.F,
		0.F,
		0.F,
	};

	// is this row exclusively additive with the current flam sequence?
	auto comma_comma_coolmeleon(const unsigned& notes) const -> bool
	{
		return (unsigned_unseen & notes) == 0U;
	}

	// to avoid keeping another float ??? idk
	auto get_dur() -> float
	{
		// cba to loop
		switch (size) {
			case 1:
				// can't have 1 row flams
				assert(0);
			case 2:
				return ms[0];
			case 3:
				return ms[0] + ms[1];
			case 4:
				return ms[0] + ms[1] + ms[2];
			default:
				assert(0);
				break;
		}
		assert(0);
		return 0.F;
	}

	void start(const float& ms_now, const unsigned& notes)
	{
		flammin = true;
		unsigned_unseen = 0U;

		grow(ms_now, notes);
	}

	void grow(const float& ms_now, const unsigned& notes)
	{
		if (size == max_flam_jammies) {
			return;
		}

		unsigned_unseen |= notes;
		ms.at(size - 1) = ms_now;

		// adjust size after setting ms, size starts at 1
		++size;
	}

	void reset()
	{
		flammin = false;
		size = 1;
	}
};

// flam detection
// used by flamjam
struct FJ_Sequencer
{
	flam flim;

	// scan for flam chords in this window
	float group_tol = 0.F;
	// tolerance for each column step
	float step_tol = 0.F;
	float mod_scaler = 0.F;

	// number of flams
	int flam_counter = 0;

	// track up to 4 flams per interval, if the number of flams exceeds this
	// number we'll just min_set (OR we could keep a moving array of flams, and
	// not flams, which would make consecutive flams much more debilitating and
	// interval proof the sequencing.. however.. it's probably not necessary to
	// get that fancy

	std::array<float, 4> mod_parts = { 1.F, 1.F, 1.F, 1.F };

	// there's too many flams already, don't bother with new sequencing and
	// shortcut into a minset in flamjammod
	// technically this means we won't start constructing sequences again until
	// the next interval.. not sure if this is desired behavior
	bool the_fifth_flammament = false;

	void set_params(const float& gt, const float& st, const float& ms)
	{
		group_tol = gt;
		step_tol = st;
		mod_scaler = ms;
	}

	void complete_seq()
	{
		if (flam_counter < max_flam_jammies) {
			mod_parts.at(flam_counter) = construct_mod_part();
			++flam_counter;
		} else {
			// bro its just flams
			the_fifth_flammament = true;
		}

		flim.reset();
	}

	auto flammin_col_check(const unsigned& notes) -> bool
	{
		// this function should never be used to start a flam
		assert(flim.flammin);

		// note : in order to prevent the last row of a quad flam from being
		// elibible to start a new flam (logically it makes no sense), instead
		// of catching full quads and resetting when we get them, we'll let them
		// pass throgh into the next note row, no matter what the row is it will
		// fail the xor check and be reset then, making only the row _after_ the
		// full flam eligible for a new start
		return flim.comma_comma_coolmeleon(notes);
	}

	// check for anything that would break the sequence
	auto flammin_tol_check(const float& ms_now) -> bool
	{
		// check if ms from last row is greater than the group tolerance
		if (ms_now > group_tol) {
			return false;
		}

		// check if the new flam duration would exceed the group tolerance with
		// the current row added
		if (flim.get_dur() + ms_now > group_tol) {
			return false;
		}

		// we may be able to continue the sequence, run the col check
		return true;
	}

	void operator()(const float& ms_now, const unsigned& notes)
	{
		// if we already have the max number of flams
		// (maybe should remove this shortcut optimization)
		// seems like we never even hit it so...
		if (the_fifth_flammament) {
			return;
		}

		// haven't started, if we're under the step tolerance, start
		if (!flim.flammin) {
			// 99.99% of cases
			if (ms_now > step_tol) {
				return;
			}
			{
				flim.start(ms_now, notes);
			}
		} else {
			// passed the tolerance checks, run the col checks
			if (flammin_tol_check(ms_now)) {

				// passed col check, advance flam
				if (flammin_col_check(notes)) {
					flim.grow(ms_now, notes);
				} else {
					// we failed the col check, but we've passed the tol checks,
					// which means this row is eligible to begin a new flam
					// sequence, complete the one that exists and start again
					complete_seq();
					flim.start(ms_now, notes);
				}
			} else {
				// reset if we exceed tolerance checks
				complete_seq();
			}
		}
	}

	void handle_interval_end()
	{
		// we probably don't want to do this, just let it build potential
		// sequences across intervals
		// flam.reset();

		the_fifth_flammament = false;
		flam_counter = 0;

		// reset everything to 1, as we build flams we will replace 1 with < 1
		// values, the more there are, the lower (stronger) the pattern mod
		mod_parts.fill(1.F);
	}

	auto construct_mod_part() -> float
	{
		// total duration of flam
		float dur = flim.get_dur();

		// scale to size of flam, we want jumpflams to punish less than quad
		// flams (while still downscaling jumptrill flams)
		// flams that register as 95% of the size adjusted window will be
		// punished less than those that register at 2%
		float dur_prop = dur / group_tol;
		dur_prop /= (static_cast<float>(flim.size) / mod_scaler);
		dur_prop = std::clamp(dur_prop, 0.F, 1.F);

		return fastsqrt(dur_prop);
	}
};
