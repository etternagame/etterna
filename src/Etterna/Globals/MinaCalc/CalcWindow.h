#pragma once
#include <array>
#include "PatternModHelpers.h"

/* custom moving window container that can do basic statistical operations on a
 * dynamic window */

static const int max_moving_window_size = 6;
template<typename T>
struct CalcMovingWindow
{
	// ok there's actually a good reason for indexing this way because it's more
	// intuitive since we are scanning row by row the earliest values in the
	// window are the oldest
	inline void operator()(const T& new_val)
	{
		// update the window
		for (int i = 1; i < max_moving_window_size; ++i) {
			_itv_vals.at(i - 1) = _itv_vals.at(i);
		}

		// set new value at size - 1
		_itv_vals.at(max_moving_window_size - 1) = new_val;
	}

	// return type T
	inline auto operator[](const int& pos) const -> T
	{
		assert(pos > 0 && pos < max_moving_window_size);
		return _itv_vals.at(pos);
	}

	// return type T
	[[nodiscard]] inline auto get_now() const -> T
	{
		return _itv_vals.back();
	}
	[[nodiscard]] inline auto get_last() const -> T
	{
		return _itv_vals.at(max_moving_window_size - 2]);
	}

	// return type T
	[[nodiscard]] inline auto get_total_for_window(const int& window) const -> T
	{
		T o = static_cast<T>(0);
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals.at(i);
		}

		return o;
	}

	// return type T
	[[nodiscard]] inline auto get_max_for_window(const int& window) const -> T
	{
		T o = static_cast<T>(0);
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o = _itv_vals.at(i) > o ? _itv_vals.at(i) : o;
		}

		return o;
	}

	// return type float
	[[nodiscard]] inline auto get_mean_of_window(const int& window) const
	  -> float
	{
		T o = static_cast<T>(0);

		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals.at(i);
		}

		return static_cast<float>(o) / static_cast<float>(window);
	}

	// return type float
	[[nodiscard]] inline auto get_total_for_windowf(const int& window) const
	  -> float
	{
		float o = 0.F;
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			o += _itv_vals.at(i);
		}

		return o;
	}

	// return type float
	[[nodiscard]] inline auto get_cv_of_window(const int& window) const -> float
	{
		float sd = 0.F;
		float avg = get_mean_of_window(window);

		// if window is 4, we check values 6/5/4/3, since this window is always
		// 6
		int i = max_moving_window_size;
		while (i > max_moving_window_size - window) {
			--i;
			sd += (static_cast<float>(_itv_vals.at(i)) - avg) *
				  (static_cast<float>(_itv_vals.at(i)) - avg);
		}

		return fastsqrt(sd / static_cast<float>(window)) / avg;
	}

	// set everything to zero
	inline void zero() { _itv_vals.fill(static_cast<T>(0)); }

	CalcWindow(T init_val = static_cast<T>(0)) { _itv_vals.fill(init_val); }

  protected:
	std::array<T, max_moving_window_size> _itv_vals;
};
