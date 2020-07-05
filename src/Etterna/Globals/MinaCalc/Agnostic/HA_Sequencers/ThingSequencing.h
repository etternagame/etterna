#pragma once
#include <cassert>

/* search for specific patterns that when chained together are extremely
 * jumptrillable, currently detection is separated, but it should probably be
 * combined */

// find [xx]a[yy]b[zz]
// used by tt_sequencer and thing 1
struct the_slip
{
	enum to_slide_or_not_to_slide
	{
		slip_unbeginninged,
		needs_single,
		needs_23_jump,
		needs_opposing_single,
		needs_opposing_ohjump,
		slip_complete
	};

	// what caused us to slip
	unsigned slip = 0;
	// are we slipping
	bool slippin_till_ya_slips_come_true = false;
	// how far those whomst'd've been slippinging
	int slide = 0;

	// ms values, 4 ms values = 5 rows, optimize by just recycling values
	// without resetting and indexing up to the size counter to get duration
	// float ms[4] = {
	//	0.f,
	//	0.f,
	//	0.f,
	//	0.f,
	//};

	// couldn't figure out how to make slip & slide work smh
	[[nodiscard]] auto the_slip_is_the_boot(const unsigned& notes) const -> bool
	{
		switch (slide) {
			// just started, need single note with no jack between our starting
			// point and [23]
			case needs_single:
				// 1100 requires 0001
				if (slip == 3 || slip == 7) {
					if (notes == 8) {
						return true;
					}
				} else
				  // if it's not a left hand jump, it's a right hand one, we
				  // need 1000
				  if (notes == 1) {
					return true;
				}
				break;
			case needs_23_jump:
				// has to be [23]
				if (notes == 6) {
					return true;
				}
				break;
			case needs_opposing_single:
				// same as 1 but reversed

				// 1100
				// 0001
				// 0110
				// requires 1000
				if (slip == 3 || slip == 7) {
					if (notes == 1) {
						return true;
					}
				} else
				  // if it's not a left hand jump, it's a right hand one, we
				  // need 0001
				  if (notes == 8) {
					return true;
				}
				break;
			case needs_opposing_ohjump:
				if (slip == 3 || slip == 7) {
					// if we started on 1100, we end on 0011
					// make detecc more inclusive i guess by allowing 0100
					if (notes == 12 || notes == 14) {
						return true;
					}
				} else
				  // starting on 0011 ends on 1100
				  // make detecc more inclusive i guess by allowing 0010
				  if (notes == 3 || notes == 7) {
					return true;
				}
				break;
			default:
				assert(0);
				break;
		}
		return false;
	}

	void start(const float& ms_now, const unsigned& notes)
	{
		slip = notes;
		slide = 0;
		slippin_till_ya_slips_come_true = true;
		grow(ms_now, notes);
	}

	void grow(const float& /*ms_now*/, const unsigned& /*notes*/)
	{
		// ms[slide] = ms_now;
		++slide;
	}

	void reset() { slippin_till_ya_slips_come_true = false; }
};

// find [12]3[24]1[34]2[13]4[12]
// used by tt2 and thing 2
struct the_slip2
{
	enum to_slide_or_not_to_slide
	{
		slip_unbeginninged,
		needs_single,
		needs_door,
		needs_blaap,
		needs_opposing_ohjump,
		slip_complete
	};

	// what caused us to slip
	unsigned slip = 0;
	// are we slipping
	bool slippin_till_ya_slips_come_true = false;
	// how far those whomst'd've been slippinging
	int slide = 0;

	// ms values, 4 ms values = 5 rows, optimize by just recycling values
	// without resetting and indexing up to the size counter to get duration
	// float ms[4] = {
	//	0.f,
	//	0.f,
	//	0.f,
	//	0.f,
	//};

	// couldn't figure out how to make slip & slide work smh
	[[nodiscard]] auto the_slip_is_the_boot(const unsigned& notes) const -> bool
	{
		switch (slide) {
			// just started, need single note with no jack between our starting
			// point and [23]
			case needs_single:
				// 1100 requires 0010
				if (slip == 3) {
					if (notes == 4) {
						return true;
					}
				} else if (notes == 2) {
					return true;
				}
				break;
			case needs_door:
				if (slip == 3) {
					if (notes == 10) {
						return true;
					}
				} else if (notes == 5) {
					return true;
				}
				break;
			case needs_blaap:
				// it's alive

				// requires 1000
				if (slip == 3) {
					if (notes == 1) {
						return true;
					}
				} else if (notes == 8) {
					return true;
				}
				break;
			case needs_opposing_ohjump:
				if (slip == 3) {
					// if we started on 1100, we end on 0011
					if (notes == 12) {
						return true;
					}
				} else
				  // starting on 0011 ends on 1100
				  if (notes == 3) {
					return true;
				}
				break;
			default:
				assert(0);
				break;
		}
		return false;
	}

	void start(const float& ms_now, const unsigned& notes)
	{
		slip = notes;
		slide = 0;
		slippin_till_ya_slips_come_true = true;
		grow(ms_now, notes);
	}

	void grow(const float& /*ms_now*/, const unsigned& /*notes*/)
	{
		// ms[slide] = ms_now;
		++slide;
	}

	void reset() { slippin_till_ya_slips_come_true = false; }
};

// sort of the same concept as fj, slightly different implementation
// used by thing1
struct TT_Sequencing
{
	the_slip fizz;
	int slip_counter = 0;
	static const int max_slips = 4;
	std::array<float, max_slips> mod_parts = { 1.F, 1.F, 1.F, 1.F };

	float scaler = 0.F;

	void set_params(const float& /*gt*/, const float& /*st*/, const float& ms)
	{
		// group_tol = gt;
		// step_tol = st;
		scaler = ms;
	}

	void complete_slip(const float& ms_now, const unsigned& notes)
	{
		if (slip_counter < max_slips) {
			mod_parts.at(slip_counter) = construct_mod_part();
		}
		++slip_counter;

		// any time we complete a slip we can start another slip, so just
		// start again
		fizz.start(ms_now, notes);
	}

	// only start if we pick up ohjump or hand with an ohjump, not a quad, not
	// singles
	static auto start_test(const unsigned& notes) -> bool
	{
		// either left hand jump or a hand containing left hand jump
		// or right hand jump or a hand containing right hand jump

		return notes == 3 || notes == 7 || notes == 12 || notes == 14;
	}

	void operator()(const float& ms_now, const unsigned& notes)
	{
		// ignore quads
		if (notes == 15) {
			// reset if we are in a sequence
			if (fizz.slippin_till_ya_slips_come_true) {
				fizz.reset();
			}
			return;
		}

		// haven't started
		if (!fizz.slippin_till_ya_slips_come_true) {
			// col check to start
			if (start_test(notes)) {
				fizz.start(ms_now, notes);
			}
			return;
		}
		// run the col checks for continuation
		if (fizz.the_slip_is_the_boot(notes)) {
			fizz.grow(ms_now, notes);
			// we found... the thing
			if (fizz.slide == 5) {
				complete_slip(ms_now, notes);
			}
		} else {
			// reset if we fail col check
			fizz.reset();
		}
	}

	void reset()
	{
		slip_counter = 0;
		mod_parts.fill(1.F);
	}

	[[nodiscard]] auto construct_mod_part() const -> float { return scaler; }
};

// sort of the same concept as fj, slightly different implementation
// used by thing2
struct TT_Sequencing2
{
	the_slip2 fizz;
	int slip_counter = 0;
	static const int max_slips = 4;
	std::array<float, max_slips> mod_parts = { 1.F, 1.F, 1.F, 1.F };

	float scaler = 0.F;

	void set_params(const float& /*gt*/, const float& /*st*/, const float& ms)
	{
		// group_tol = gt;
		// step_tol = st;
		scaler = ms;
	}

	void complete_slip(const float& ms_now, const unsigned& notes)
	{
		if (slip_counter < max_slips) {
			mod_parts.at(slip_counter) = construct_mod_part();
		}
		++slip_counter;

		// any time we complete a slip we can start another slip, so just
		// start again
		fizz.start(ms_now, notes);
	}

	// only start if we pick up ohjump or hand with an ohjump, not a quad, not
	// singles
	static auto start_test(const unsigned& notes) -> bool
	{
		// either left hand jump or a hand containing left hand jump
		// or right hand jump or a hand containing right hand jump
		return notes == 3 || notes == 12;
	}

	void operator()(const float& ms_now, const unsigned& notes)
	{
		// ignore quads
		if (notes == 15) {
			// reset if we are in a sequence
			if (fizz.slippin_till_ya_slips_come_true) {
				fizz.reset();
			}
			return;
		}

		// haven't started
		if (!fizz.slippin_till_ya_slips_come_true) {
			// col check to start
			if (start_test(notes)) {
				fizz.start(ms_now, notes);
			}
			return;
		}
		// run the col checks for continuation
		if (fizz.the_slip_is_the_boot(notes)) {
			fizz.grow(ms_now, notes);
			// we found... the thing
			if (fizz.slide == 5) {
				complete_slip(ms_now, notes);
			}
		} else {
			// reset if we fail col check
			fizz.reset();
		}
	}

	void reset()
	{
		slip_counter = 0;
		mod_parts.fill(1.F);
	}

	[[nodiscard]] auto construct_mod_part() const -> float { return scaler; }
};
