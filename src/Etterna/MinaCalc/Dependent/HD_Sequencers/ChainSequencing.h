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
	int cur_chain_single_taps = 0;
	int cur_chain_ohj = 0;
	int _len = 0;
	int chain_swaps = 0;
	bool chain_swapping = false;

	int _max_len = 0;
	int max_chain_single_taps = 0;
	int max_chain_ohj = 0;
	int max_chain_swaps = 0;
	col_type chaining_col = col_init;

	void zero()
	{
		cur_chain_single_taps = 0;
		cur_chain_ohj = 0;
		chain_swaps = 0;
		_len = 0;
		chaining_col = col_init;
		_max_len = 0;
		max_chain_single_taps = 0;
		max_chain_ohj = 0;
		max_chain_swaps = 0;
		chain_swapping = false;
	}

	int get_largest_chain_single_taps()
	{
		return cur_chain_single_taps > max_chain_single_taps ? cur_chain_single_taps
													  : max_chain_single_taps;
	}

	int get_largest_chain_ohj()
	{
		return cur_chain_ohj > max_chain_ohj ? cur_chain_ohj : max_chain_ohj;
	}

	void complete_seq()
	{
		assert(_len >= 0);

		// remove this check to consider 1111122222 a chain
		// otherwise, only 11111[12]22222 is a chain
		if (chain_swaps != 0) {
			// find a way to break this
			max_chain_single_taps = get_largest_chain_single_taps();
			max_chain_ohj = get_largest_chain_ohj();
			_max_len = _len > _max_len ? _len : _max_len;
			max_chain_swaps =
			  chain_swaps > max_chain_swaps ? chain_swaps : max_chain_swaps;
		}

		// reset curr chain info
		cur_chain_single_taps = 0;
		cur_chain_ohj = 0;
		_len = 0;
		chaining_col = col_init;
		chain_swaps = 0;
		chain_swapping = false;
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
				chaining_col = ct;
				cur_chain_single_taps++;
				_len++;
				break;
			case base_single_jump:
				// 1[12] or 2[12]
				// chain expects a swap in columns
				// or may complete
				chaining_col = last_ct; // set to the column we used to be on
				cur_chain_ohj++;
				_len++;
				chain_swapping = true;
				break;
			case base_jump_single:
				// [12]1 or [12]2
				// chain continuing
				// check to see if it is valid to continue
				if (chain_swapping) {
					chain_swapping = false;
					chain_swaps++;
				}

				if ((chaining_col == col_left && ct == col_right) ||
					(chaining_col == col_right && ct == col_left)) {
					// valid to swap columns and continue
					chaining_col = ct;
					cur_chain_single_taps++;
					_len++;
				} else if (_len >= 2) {
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

};
