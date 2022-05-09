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
	[[nodiscard]] auto Ago() const -> float;
	void Touch() { tm = std::chrono::steady_clock::now(); }
	[[nodiscard]] auto IsZero() const -> bool
	{
		return tm == std::chrono::steady_clock::time_point();
	}
	void SetZero() { tm = std::chrono::steady_clock::time_point(); }

	/* Time between last call to GetDeltaTime() (Ago() + Touch()): */
	auto GetDeltaTime() -> float;
	/* Alias for Ago */
	[[nodiscard]] auto PeekDeltaTime() const -> float { return Ago(); }

	static auto GetTimeSinceStart()
	  -> float; // seconds since the program was started

	/* Add (or subtract) a duration from a timestamp.  The result is another
	 * timestamp. */
	auto operator+(float tm) const -> RageTimer;
	auto operator-(float tm) const -> RageTimer { return *this + -tm; }
	void operator+=(float tm) { *this = *this + tm; }
	void operator-=(float tm) { *this = *this + -tm; }

	/* Find the amount of time between two timestamps.  The result is a
	 * duration. */
	auto operator-(const RageTimer& rhs) const -> float;

	auto operator<(const RageTimer& rhs) const -> bool;
	std::chrono::steady_clock::time_point tm;

  private:
	static auto Sum(const RageTimer& lhs, float tm) -> RageTimer;
	static auto Difference(const RageTimer& lhs, const RageTimer& rhs) -> float;
};

extern const RageTimer RageZeroTimer;

#endif
