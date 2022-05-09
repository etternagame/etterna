#pragma once

/// multiply the time between previous jacks in the chain by this
/// if the gap is bigger than that, it is a new chain/anchor
constexpr float chain_slowdown_scale_threshold = 1.99F;

/** Looks for anchors and chains.
* this:
*
* 01
* 01
* 11
* 10
* 10
* 11
* 01
* 01 ...
*
* or
* 01
* 11
* 11
* 10 ...
*
* special case: also this
* (not a chain!)
* 01
* 11
* 11
* 01 ...
*/
struct CJ_OHAnchor_Sequencer
{
	int chain_swaps = 0;
	int not_swaps = 0;
	int cur_len = 0;
	int cur_anchor_len = 0;
	bool chain_swapping = false;

	int max_chain_swaps = 0;
	int max_not_swaps = 0;
	int max_total_len = 0;
	int max_anchor_len = 0;
	col_type anchor_col = col_init;
	float last_ms = ms_init;

	void zero()
	{
		reset_max_seq();
		reset_seq();
		last_ms = ms_init;
	}

	void reset_seq() {
		chain_swaps = 0;
		not_swaps = 0;
		cur_len = 0;
		cur_anchor_len = 0;

		chain_swapping = false;
		anchor_col = col_init;
		// not resetting ms time here
	}

	void reset_max_seq() {
		max_chain_swaps = 0;
		max_not_swaps = 0;
		max_total_len = 0;
		max_anchor_len = 0;
	}

	void update_max_seq() {
		max_chain_swaps = get_max_chain_swaps();
		max_not_swaps = get_max_not_swaps();
		max_total_len = get_max_total_len();
		max_anchor_len = get_max_anchor_len();
	}

	/// to reset the current chain and continue a new one
	void complete_seq()
	{
		assert(cur_len >= 0);

		// if a chain swap was in progress
		// we ended on a jump -- the gap after was too big.
		// to capture the difficulty of the jump, fail a swap
		if (chain_swapping) {
			swap_failed();
		}

		// remove this check to consider 1111122222 a chain
		// otherwise, only 11111[12]22222 is a chain
		// and 11111[12]11111 also qualifies
		if (chain_swaps != 0 || not_swaps != 0) {
			update_max_seq();
		}

		// reset curr chain info
		reset_seq();
	}

	/// execute a chain swap. set longest anchor size, etc
	/// base pattern required for successful chain swap:
	/// 01
	/// 11
	/// 10
	void chain_swap()
	{
		chain_swapping = false;
		max_anchor_len = get_max_anchor_len();
		cur_anchor_len = 0;
		chain_swaps++;
	}

	/// technically difficult repeat anchor-jumps
	/// base pattern:
	/// 01
	/// 11
	/// 01
	void swap_failed()
	{
		chain_swapping = false;
		not_swaps++;
		// anchor continues ...
	}

	void operator()(const col_type& ct, const base_type& bt, const col_type& last_ct, const float& any_ms)
	{
		if (last_ms * chain_slowdown_scale_threshold < any_ms) {
			// if the taps were too slow, reset
			complete_seq();
		}
		last_ms = any_ms;

		switch (bt) {
			case base_left_right:
			case base_right_left:
				// not a chain ...
				// not an anchor ...
				// end it
				complete_seq();
				break;
			case base_jump_jump:
				// allow [12][12] to continue a chain
				// anchor_col does not change
				cur_len++;
				cur_anchor_len++;

				// starting with jumps? no thanks
				if (anchor_col != col_init)
					chain_swapping = true;
				break;
			case base_single_single:
				// consecutive 11 or 22
				// mid chain or about to chain
				anchor_col = ct;
				cur_len++;
				cur_anchor_len++;
				break;
			case base_single_jump:
				// 1[12] or 2[12]
				// chain expects a swap in columns
				// or may complete
				anchor_col = last_ct; // set to the column we used to be on
				cur_len++;
				chain_swapping = true;
				break;
			case base_jump_single:
				// [12]1 or [12]2
				// chain continuing
				cur_len++;
				cur_anchor_len++;

				if ((anchor_col == col_left && ct == col_right) ||
					(anchor_col == col_right && ct == col_left)) {
					// valid to swap columns and continue

					// cope with swaps
					if (chain_swapping) {
						chain_swap();
					}
				} else {
					// anchor is continuing
					// jump was just ... a jump

					// was trying to swap but failed
					if (chain_swapping) {
						swap_failed();
					}
				}
				// this is currently the anchoring column
				anchor_col = ct;

				break;
			case base_type_init:
				// no info
				break;
			default:
				assert(0);
				break;
		}
		
	}

	int get_max_total_len() {
		return cur_len > max_total_len ? cur_len : max_total_len;
	}

	int get_max_anchor_len() {
		return cur_anchor_len > max_anchor_len ? cur_anchor_len
											   : max_anchor_len;
	}

	int get_max_chain_swaps() {
		return chain_swaps > max_chain_swaps ? chain_swaps : max_chain_swaps;
	}

	int get_max_not_swaps() {
		return not_swaps > max_not_swaps ? not_swaps : max_not_swaps;
	}
};
