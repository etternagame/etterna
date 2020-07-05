#pragma once

// one hand jump sequencer, not complex enough to require its own pattern struct
// used by ohjmod
// note we do want the occasional single ohjump in js to count as a sequence of
// length 1
struct OHJ_Sequencer
{
	// COUNT TAPS IN JUMPS
	int cur_seq_taps = 0;
	int max_seq_taps = 0;

	auto get_largest_seq_taps() -> int
	{
		return cur_seq_taps > max_seq_taps ? cur_seq_taps : max_seq_taps;
	}

	void complete_seq()
	{
		// negative values should not be possible
		assert(cur_seq_taps >= 0);

		// set the largest ohj sequence
		max_seq_taps = get_largest_seq_taps();
		// reset
		cur_seq_taps = 0;
	}

	void zero()
	{
		cur_seq_taps = 0;
		max_seq_taps = 0;
	}

	void operator()(const col_type& ct, const base_type& bt)
	{
		if (cur_seq_taps == 0) {
			// if we aren't in a sequence and aren't going to start one, bail
			if (ct != col_ohjump) {
				return;
			}
			{ // allow sequences of 1 by starting any time we hit an ohjump
				cur_seq_taps += 2;
			}
		}

		// we know between the following that the latter is more
		// difficult [12][12][12]222[12][12][12]
		// [12][12][12]212[12][12][12]
		// so we want to penalize not only any break in the ohj
		// sequence but further penalize breaks which contain
		// cross column taps this should also reflect the
		// difference between [12]122[12], [12]121[12] cases
		// like 121[12][12]212[12][12]121 should probably have
		// some penalty but likely won't with this setup, but
		// everyone hates that anyway and it would be quite
		// difficult to force the current setup to do so without
		// increasing complexity significantly (probably)

		// don't reset immediately on a single note, wait to see
		// what comes next, if now.last_cc == base_jump_single, we have just
		// broken a sequence (technically this can be simply something like
		// [12]2[12]2[12]2 so the ohjumps wouldn't really be a sequence
		switch (bt) {
			case base_jump_jump:
				// continuing sequence
				cur_seq_taps += 2;
				break;
			case base_jump_single:
				// just came out of a jump seq, do nothing... wait to see what
				// happens
				break;
			case base_left_right:
			case base_right_left:
				// if we have an actual cross column tap now, and if we just
				// came from a jump -> single, then we have something like
				// [12]21, which is much harder than [12]22, so penalize the
				// sequence slightly before completing
				if (cur_seq_taps == 2) {
					cur_seq_taps -= 1;
				} else {
					cur_seq_taps -= 3;
				}
				complete_seq();
				break;
			case base_single_single:
				// we have something like [12]22, complete the sequence
				// without the penalty that the cross column incurs
				complete_seq();
				break;
			case base_single_jump:
				// [12]1[12]... we broke a sequence and went right back into
				// one.. reset sequence for now but come back to revsit this, we
				// might want to have different behavior, but we'd need to track
				// the columns of the single notes in the chain
				// if (now.last_cc == base_jump_single)
				//	complete_seq();
				// else
				complete_seq();
				break;
			case base_type_init:
				// do nothing, we don't have enough info yet
				break;
			default:
				assert(0);
				break;
		}
	}
};
