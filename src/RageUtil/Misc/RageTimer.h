/* RageTimer - Timer services. */

#ifndef RAGE_TIMER_H
#define RAGE_TIMER_H

#include <chrono>

class RageTimer
{
  public:
	RageTimer() { Touch(); }
	RageTimer(std::chrono::steady_clock::time_point tm) : tm(tm) {};
	RageTimer(unsigned microseconds) : tm()
	{
		tm += std::chrono::microseconds(microseconds);
	}
	RageTimer(unsigned secs, unsigned microseconds) : tm()
	{
		auto seconds = std::chrono::seconds(secs);
		auto microsecs = std::chrono::microseconds(microseconds);
		tm += seconds + microsecs;
	}

	/* Time ago this RageTimer represents. */
	[[nodiscard]] auto Ago() const -> double;
	void Touch() { tm = std::chrono::steady_clock::now(); }
	[[nodiscard]] auto IsZero() const -> bool
	{
		return tm == std::chrono::steady_clock::time_point();
	}
	void SetZero() { tm = std::chrono::steady_clock::time_point(); }

	/* Time between last call to GetDeltaTime() (Ago() + Touch()): */
	auto GetDeltaTime() -> double;
	/* Alias for Ago */
	[[nodiscard]] auto PeekDeltaTime() const -> double { return Ago(); }

	static auto GetTimeSinceStart()
	  -> double; // seconds since the program was started

	static auto GetTimeSinceStart64() -> double;

	/* Add (or subtract) a duration from a timestamp.  The result is another
	 * timestamp. */
	auto operator+(double tm) const -> RageTimer;
	auto operator-(double tm) const -> RageTimer { return *this + -tm; }
	void operator+=(double tm) { *this = *this + tm; }
	void operator-=(double tm) { *this = *this + -tm; }

	/* Find the amount of time between two timestamps.  The result is a
	 * duration. */
	auto operator-(const RageTimer& rhs) const -> double;

	auto operator<(const RageTimer& rhs) const -> bool;
	std::chrono::steady_clock::time_point tm;

  private:
	static auto Sum(const RageTimer& lhs, double tm) -> RageTimer;
	static auto Difference(const RageTimer& lhs, const RageTimer& rhs) -> double;
};

extern const RageTimer RageZeroTimer;

#endif
