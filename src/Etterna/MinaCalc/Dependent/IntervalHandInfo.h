#pragma once
#include "../CalcWindow.h"

// accumulates hand specific info across an interval as it's processed by row
struct ItvHandInfo
{
	void set_col_taps(const col_type& col)
	{
		// this could be more efficient but at least it's clear (ish)?
		switch (col) {
			case col_left:
			case col_right:
				++_col_taps.at(col);
				break;
			case col_ohjump:
				++_col_taps.at(col_left);
				++_col_taps.at(col_right);
				_col_taps.at(col) += 2;
				break;
			default:
				assert(0);
				break;
		}
	}

	// handle end of interval behavior here
	void interval_end()
	{
		// update interval mw for hand taps
		_mw_hand_taps(_col_taps.at(col_left) + _col_taps.at(col_right));

		// update interval mws for col taps
		for (auto& ct : ct_loop) {
			_mw_col_taps.at(ct)(_col_taps.at(ct));
		}

		// reset taps per col on this hand
		_col_taps.fill(0);
	}

	// zeroes out all values for everything, complete reset for when we swap
	// hands maybe move to constructor and reconstruct when swapping hands??
	void zero()
	{
		_col_taps.fill(0);

		for (auto& mw : _mw_col_taps) {
			mw.zero();
		}
		_mw_hand_taps.zero();
	}

	/* access functions for col tap counts */
	[[nodiscard]] auto get_col_taps_nowi(const col_type& ct) const -> int
	{
		assert(ct < num_col_types);
		return _mw_col_taps.at(ct).get_now();
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] auto get_col_taps_nowf(const col_type& ct) const -> float
	{
		assert(ct < num_col_types);
		return static_cast<float>(_mw_col_taps.at(ct).get_now());
	}

	[[nodiscard]] auto get_col_taps_windowi(const col_type& ct,
											const int& window) const -> int
	{
		assert(ct < num_col_types && window < max_moving_window_size);
		return _mw_col_taps.at(ct).get_total_for_window(window);
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] auto get_col_taps_windowf(const col_type& ct,
											const int& window) const -> float
	{
		assert(ct < num_col_types && window < max_moving_window_size);
		return static_cast<float>(
		  _mw_col_taps.at(ct).get_total_for_window(window));
	}

	// col operations
	[[nodiscard]] auto cols_equal_now() const -> bool
	{
		return get_col_taps_nowi(col_left) == get_col_taps_nowi(col_right);
	}

	[[nodiscard]] auto cols_equal_window(const int& window) const -> bool
	{
		return get_col_taps_windowi(col_left, window) ==
			   get_col_taps_windowi(col_right, window);
	}

	[[nodiscard]] auto get_col_prop_high_by_low() const -> float
	{
		return div_high_by_low(get_col_taps_nowf(col_left),
							   get_col_taps_nowf(col_right));
	}

	[[nodiscard]] auto get_col_prop_low_by_high() const -> float
	{
		return div_low_by_high(get_col_taps_nowf(col_left),
							   get_col_taps_nowf(col_right));
	}

	[[nodiscard]] auto get_col_prop_high_by_low_window(const int& window) const
	  -> float
	{
		return div_high_by_low(get_col_taps_windowf(col_left, window),
							   get_col_taps_windowf(col_right, window));
	}

	[[nodiscard]] auto get_col_prop_low_by_high_window(const int& window) const
	  -> float
	{
		return div_low_by_high(get_col_taps_windowf(col_left, window),
							   get_col_taps_windowf(col_right, window));
	}

	[[nodiscard]] auto get_col_diff_high_by_low() const -> int
	{
		return diff_high_by_low(get_col_taps_nowi(col_left),
								get_col_taps_nowi(col_right));
	}

	[[nodiscard]] auto get_col_diff_high_by_low_window(const int& window) const
	  -> int
	{
		return diff_high_by_low(get_col_taps_windowi(col_left, window),
								get_col_taps_windowi(col_right, window));
	}

	/* access functions for hand tap counts */

	[[nodiscard]] auto get_taps_nowi() const -> int
	{
		return _mw_hand_taps.get_now();
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] auto get_taps_nowf() const -> float
	{
		return static_cast<float>(_mw_hand_taps.get_now());
	}

	[[nodiscard]] auto get_taps_windowi(const int& window) const -> int
	{
		assert(window < max_moving_window_size);
		return _mw_hand_taps.get_total_for_window(window);
	}

	// cast to float for divisioning and clean screen
	[[nodiscard]] auto get_taps_windowf(const int& window) const -> float
	{
		assert(window < max_moving_window_size);
		return static_cast<float>(_mw_hand_taps.get_total_for_window(window));
	}

  private:
	std::array<int, num_col_types> _col_taps = { 0, 0, 0 };

	// switch to keeping generic moving windows here, if any mod needs a moving
	// window query for anything here, we've already saved computation. any mod
	// that needs custom moving windows based on sequencing will have to keep
	// its own container, but otherwise these should be referenced
	std::array<CalcMovingWindow<int>, num_col_types> _mw_col_taps;
	CalcMovingWindow<int> _mw_hand_taps;
};
