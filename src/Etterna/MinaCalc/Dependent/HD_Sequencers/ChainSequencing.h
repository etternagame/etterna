#pragma once

/* Looks for chains.
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
*/
struct Chain_Sequencer
{
	int chain_swaps = 0;
	int cur_len = 0;
	int cur_anchor_len = 0;
	bool chain_swapping = false;

	int max_chain_swaps = 0;
	int max_total_len = 0;
	int max_anchor_len = 0;
	col_type anchor_col = col_init;

	void zero()
	{
		reset_max_seq();
		reset_seq();
	}

	void reset_seq() {
		chain_swaps = 0;
		cur_len = 0;
		cur_anchor_len = 0;

		chain_swapping = false;
		anchor_col = col_init;
	}

	void reset_max_seq() {
		max_chain_swaps = 0;
		max_total_len = 0;
		max_anchor_len = 0;
	}

	void update_max_seq() {
		max_chain_swaps = get_max_chain_swaps();
		max_total_len = get_max_total_len();
		max_anchor_len = get_max_anchor_len();
	}

	/// to reset the current chain and continue a new one
	void complete_seq()
	{
		assert(cur_len >= 0);

		// remove this check to consider 1111122222 a chain
		// otherwise, only 11111[12]22222 is a chain
		if (chain_swaps != 0) {
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

	void operator()(const col_type& ct, const base_type& bt, const col_type& last_ct)
	{
		switch (bt) {
			case base_jump_jump:
			case base_left_right:
			case base_right_left:
				// not exactly a chain ...
				// end it
				complete_seq();
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
				// check to see if it is valid to continue
				if (chain_swapping) {
					chain_swap();
				}

				if ((anchor_col == col_left && ct == col_right) ||
					(anchor_col == col_right && ct == col_left)) {
					// valid to swap columns and continue
					anchor_col = ct;
					cur_len++;
					cur_anchor_len++;
				} else if (cur_len >= 2) {
					// require length to be at least 2
					// otherwise we dont really know anything
					// if sufficient length, chain is broken
					// this is an anchor with ohj mixed.
					complete_seq();
				}
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
};
