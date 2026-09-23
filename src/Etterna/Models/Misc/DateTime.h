#ifndef DATE_TIME_H
#define DATE_TIME_H

#include "EnumHelper.h"
#include <compare>
#include <ctime>

auto
StringToDayInYear(const std::string& sDayInYear) -> int;

/** @brief The number of days we check for previously. */
const int NUM_LAST_DAYS = 7;
/** @brief The number of weeks we check for previously. */
const int NUM_LAST_WEEKS = 52;
/**
 * @brief The number of days that are in a year.
 *
 * This is set up to be a maximum for leap years. */
const int DAYS_IN_YEAR = 366;
/**
 * @brief The number of hours in a day. */
const int HOURS_IN_DAY = 24;
/**
 * @brief The number of days that are in a week. */
const int DAYS_IN_WEEK = 7;
enum Month
{
	Month_January,
	Month_February,
	Month_March,
	Month_April,
	Month_May,
	Month_June,
	Month_July,
	Month_August,
	Month_September,
	Month_October,
	Month_November,
	Month_December,
	NUM_Month = 12, /**< The number of months in the year. */
	Month_Invalid	/**< There should be no month at this point. */
};

auto
DayInYearToString(int iDayInYearIndex) -> std::string;
auto
LastDayToString(int iLastDayIndex) -> std::string;
auto
LastDayToLocalizedString(int iLastDayIndex) -> std::string;
auto
DayOfWeekToString(int iDayOfWeekIndex) -> std::string;
auto
DayOfWeekToLocalizedString(int iDayOfWeekIndex) -> std::string;
auto
HourInDayToString(int iHourIndex) -> std::string;
auto
HourInDayToLocalizedString(int iHourIndex) -> std::string;
auto
MonthToString(Month month) -> const std::string&;
auto
MonthToLocalizedString(Month month) -> const std::string&;
auto
LastWeekToString(int iLastWeekIndex) -> std::string;
auto
LastWeekToLocalizedString(int iLastWeekIndex) -> std::string;
LuaDeclareType(Month);

auto
AddDays(tm start, int iDaysToMove) -> tm;
auto
GetYesterday(tm start) -> tm;
auto
GetDayOfWeek(tm time) -> int;
auto
GetNextSunday(tm start) -> tm;

auto
GetDayInYearAndYear(int iDayInYearIndex, int iYear) -> tm;

/** @brief A standard way of determining the date and the time. */
struct DateTime
{
	/**
	 * @brief The number of seconds after the minute.
	 *
	 * Valid values are [0, 59]. */
	int tm_sec;
	/**
	 * @brief The number of minutes after the hour.
	 *
	 * Valid values are [0, 59]. */
	int tm_min;
	/**
	 * @brief The number of hours since midnight (or 0000 hours).
	 *
	 * Valid values are [0, 23]. */
	int tm_hour;
	/**
	 * @brief The specified day of the current month.
	 *
	 * Valid values are [1, 31].
	 *
	 * XXX: Is it possible to set an illegal date through here,
	 * such as day 30 of February? -Wolfman2000 */
	int tm_mday;
	/**
	 * @brief The number of months since January.
	 *
	 * Valid values are [0, 11]. */
	int tm_mon;
	/** @brief The number of years since the year 1900. */
	int tm_year;

	/** @brief Set up a default date and time. */
	DateTime();
	/** @brief Initialize the date and time. */
	void Init();

	/**
	 * @brief Determine if this DateTime is equal to some other time.
	 * @param other the other DateTime to check.
	 * @return true if this is equal to the other time, or false otherwise. */
	auto operator==(const DateTime& other) const -> bool = default;

	/**
	 * @brief Chronologically compares this DateTime with another.
	 * @param other The DateTime to compare against.
	 * @return A std::strong_ordering indicating whether this DateTime is
	 * earlier, equivalent, or later. */
	auto operator<=>(const DateTime& other) const -> std::strong_ordering
	{
		if (auto cmp = tm_year <=> other.tm_year; cmp != 0)
			return cmp;
		if (auto cmp = tm_mon <=> other.tm_mon; cmp != 0)
			return cmp;
		if (auto cmp = tm_mday <=> other.tm_mday; cmp != 0)
			return cmp;
		if (auto cmp = tm_hour <=> other.tm_hour; cmp != 0)
			return cmp;
		if (auto cmp = tm_min <=> other.tm_min; cmp != 0)
			return cmp;
		return tm_sec <=> other.tm_sec;
	}

	/**
	 * @brief Retrieve the current date and time.
	 * @return the current date and time. */
	static auto GetNowDateTime() -> DateTime;
	/**
	 * @brief Retrieve the current date.
	 * @return the current date. */
	static auto GetNowDate() -> DateTime;

	static auto GetFromString(const std::string& str) -> DateTime;

	static auto GetYesterday() -> DateTime;

	/** @brief Remove the time portion from the date. */
	void StripTime();

	/**
	 * @brief Retrieve a string representation of the current date and time.
	 *
	 * This returns a common SQL/XML format: "YYYY-MM-DD HH:MM:SS".
	 * @return the string representation of the date and time. */
	[[nodiscard]] auto GetString() const -> std::string;
	/**
	 * @brief Attempt to turn a string into a DateTime.
	 *
	 * @param sDateTime the string to attempt to convert.
	 * @return true if the conversion worked, or false otherwise. */
	auto FromString(const std::string& sDateTime) -> bool;
};

#endif
