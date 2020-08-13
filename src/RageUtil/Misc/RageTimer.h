/* RageTimer - Timer services. */

#ifndef RAGE_TIMER_H
#define RAGE_TIMER_H

#include <chrono>

class RageTimer
{
  public:
	RageTimer() { Touch(); }
	RageTimer(unsigned microseconds)
	{
		this->c_dur = std::chrono::microseconds(microseconds);
	}
	RageTimer(unsigned secs, unsigned microseconds)
	{
		auto seconds = std::chrono::seconds(secs);
		auto microsecs = std::chrono::microseconds(microseconds);
		this->c_dur = seconds + microsecs;
	}

	/* Time ago this RageTimer represents. */
	[[nodiscard]] auto Ago() const -> float;
	void Touch();
	[[nodiscard]] auto IsZero() const -> bool
	{
		return this->c_dur == std::chrono::microseconds::zero();
	}
	void SetZero() { this->c_dur = std::chrono::microseconds::zero(); }

	/* Time between last call to GetDeltaTime() (Ago() + Touch()): */
	auto GetDeltaTime() -> float;
	/* Alias for Ago */
	[[nodiscard]] auto PeekDeltaTime() const -> float { return Ago(); }

	static auto GetTimeSinceStart()
	  -> float; // seconds since the program was started
	static auto GetUsecsSinceStart()
	  -> uint64_t; // microseconds since the program was started

	/* Get a timer representing half of the time ago as this one. */
	[[nodiscard]] auto Half() const -> RageTimer;

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
	std::chrono::microseconds c_dur{};

  private:
	static auto Sum(const RageTimer& lhs, float tm) -> RageTimer;
	static auto Difference(const RageTimer& lhs, const RageTimer& rhs) -> float;
};

extern const RageTimer RageZeroTimer;

// For profiling how long some chunk of code takes. -Kyz
#define START_TIME(name)                                                       \
	uint64_t name##_start_time = RageTimer::GetUsecsSinceStart();
#define START_TIME_CALL_COUNT(name)                                            \
	START_TIME(name);                                                          \
	++name##_call_count;
#define END_TIME(name)                                                         \
	uint64_t name##_end_time = RageTimer::GetUsecsSinceStart();                \
	LOG->Time(#name " time: %zu to %zu = %zu",                                 \
			  name##_start_time,                                               \
			  name##_end_time,                                                 \
			  name##_end_time - name##_start_time);
#define END_TIME_ADD_TO(name)                                                  \
	uint64_t name##_end_time = RageTimer::GetUsecsSinceStart();                \
	name##_total += name##_end_time - name##_start_time;
#define END_TIME_CALL_COUNT(name)                                              \
	END_TIME_ADD_TO(name);                                                     \
	++name##_end_count;

#define DECL_TOTAL_TIME(name) extern uint64_t name##_total;
#define DEF_TOTAL_TIME(name) uint64_t name##_total = 0;
#define PRINT_TOTAL_TIME(name)                                                 \
	LOG->Time(#name " total time: %zu", name##_total);
#define DECL_TOT_CALL_PAIR(name)                                               \
	extern uint64_t name##_total;                                              \
	extern uint64_t name##_call_count;
#define DEF_TOT_CALL_PAIR(name)                                                \
	uint64_t name##_total = 0;                                                 \
	uint64_t name##_call_count = 0;
#define PRINT_TOT_CALL_PAIR(name)                                              \
	LOG->Time(#name " calls: %zu, time: %zu, per: %f",                         \
			  name##_call_count,                                               \
			  name##_total,                                                    \
			  static_cast<float>(name##_total) / name##_call_count);
#define DECL_TOT_CALL_END(name)                                                \
	DECL_TOT_CALL_PAIR(name);                                                  \
	extern uint64_t name##_end_count;
#define DEF_TOT_CALL_END(name)                                                 \
	DEF_TOT_CALL_PAIR(name);                                                   \
	uint64_t name##_end_count = 0;
#define PRINT_TOT_CALL_END(name)                                               \
	LOG->Time(#name " calls: %zu, time: %zu, early end: %zu, per: %f",         \
			  name##_call_count,                                               \
			  name##_total,                                                    \
			  name##_end_count,                                                \
			  static_cast<float>(name##_total) /                               \
				(name##_call_count - name##_end_count));

#endif
