#include "Etterna/Globals/global.h"
#include "DateTime.h"
#include "EnumHelper.h"
#include "LocalizedString.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Utils/RageUtil.h"

DateTime::DateTime()
{
	Init();
}

void
DateTime::Init()
{
	ZERO(*this);
}

bool
DateTime::operator<(const DateTime& other) const
{
#define COMPARE(v)                                                             \
	if ((v) != other.v)                                                        \
		return (v) < other.v;
	COMPARE(tm_year);
	COMPARE(tm_mon);
	COMPARE(tm_mday);
	COMPARE(tm_hour);
	COMPARE(tm_min);
	COMPARE(tm_sec);
#undef COMPARE
	// they're equal
	return false;
}

bool
DateTime::operator==(const DateTime& other) const
{
#define COMPARE(x)                                                             \
	if ((x) != other.x)                                                        \
		return false;
	COMPARE(tm_year);
	COMPARE(tm_mon);
	COMPARE(tm_mday);
	COMPARE(tm_hour);
	COMPARE(tm_min);
	COMPARE(tm_sec);
#undef COMPARE
	return true;
}

bool
DateTime::operator>(const DateTime& other) const
{
#define COMPARE(v)                                                             \
	if ((v) != other.v)                                                        \
		return (v) > other.v;
	COMPARE(tm_year);
	COMPARE(tm_mon);
	COMPARE(tm_mday);
	COMPARE(tm_hour);
	COMPARE(tm_min);
	COMPARE(tm_sec);
#undef COMPARE
	// they're equal
	return false;
}

DateTime
DateTime::GetNowDateTime()
{
	auto now = time(nullptr);
	tm tNow;
	localtime_r(&now, &tNow);
	DateTime dtNow;
#define COPY_M(v) dtNow.v = tNow.v;
	COPY_M(tm_year);
	COPY_M(tm_mon);
	COPY_M(tm_mday);
	COPY_M(tm_hour);
	COPY_M(tm_min);
	COPY_M(tm_sec);
#undef COPY_M
	return dtNow;
}

DateTime
DateTime::GetNowDate()
{
	auto tNow = GetNowDateTime();
	tNow.StripTime();
	return tNow;
}

void
DateTime::StripTime()
{
	tm_hour = 0;
	tm_min = 0;
	tm_sec = 0;
}

// Common SQL/XML format: "YYYY-MM-DD HH:MM:SS"
std::string
DateTime::GetString() const
{
	auto s = ssprintf("%d-%02d-%02d", tm_year + 1900, tm_mon + 1, tm_mday);

	if (tm_hour != 0 || tm_min != 0 || tm_sec != 0) {
		s += ssprintf(" %02d:%02d:%02d", tm_hour, tm_min, tm_sec);
	}

	return s;
}

bool
DateTime::FromString(const std::string& sDateTime)
{
	Init();

	int ret;

	ret = sscanf(sDateTime.c_str(),
				 "%d-%d-%d %d:%d:%d",
				 &tm_year,
				 &tm_mon,
				 &tm_mday,
				 &tm_hour,
				 &tm_min,
				 &tm_sec);
	if (ret != 6) {
		ret =
		  sscanf(sDateTime.c_str(), "%d-%d-%d", &tm_year, &tm_mon, &tm_mday);
		if (ret != 3) {
			return false;
		}
	}

	tm_year -= 1900;
	tm_mon -= 1;
	return true;
}

std::string
DayInYearToString(int iDayInYear)
{
	return ssprintf("DayInYear%03d", iDayInYear);
}

int
StringToDayInYear(const std::string& sDayInYear)
{
	int iDayInYear;
	if (sscanf(sDayInYear.c_str(), "DayInYear%d", &iDayInYear) != 1)
		return -1;
	return iDayInYear;
}

static const std::string LAST_DAYS_NAME[NUM_LAST_DAYS] = {
	"Today", "Yesterday", "Day2Ago", "Day3Ago", "Day4Ago", "Day5Ago", "Day6Ago",
};

std::string
LastDayToString(int iLastDayIndex)
{
	return LAST_DAYS_NAME[iLastDayIndex];
}

static const char* DAY_OF_WEEK_TO_NAME[DAYS_IN_WEEK] = {
	"Sunday",	"Monday", "Tuesday",  "Wednesday",
	"Thursday", "Friday", "Saturday",
};

std::string
DayOfWeekToString(int iDayOfWeekIndex)
{
	return DAY_OF_WEEK_TO_NAME[iDayOfWeekIndex];
}

std::string
HourInDayToString(int iHourInDayIndex)
{
	return ssprintf("Hour%02d", iHourInDayIndex);
}

static const char* MonthNames[] = {
	"January", "February", "March",		"April",   "May",	   "June",
	"July",	   "August",   "September", "October", "November", "December",
};
XToString(Month);
XToLocalizedString(Month);
LuaXType(Month);

std::string
LastWeekToString(int iLastWeekIndex)
{
	switch (iLastWeekIndex) {
		case 0:
			return "ThisWeek";
			break;
		case 1:
			return "LastWeek";
			break;
		default:
			return ssprintf("Week%02dAgo", iLastWeekIndex);
			break;
	}
}

std::string
LastDayToLocalizedString(int iLastDayIndex)
{
	auto s = LastDayToString(iLastDayIndex);
	s_replace(s, "Day", "");
	s_replace(s, "Ago", " Ago");
	return s;
}

std::string
LastWeekToLocalizedString(int iLastWeekIndex)
{
	auto s = LastWeekToString(iLastWeekIndex);
	s_replace(s, "Week", "");
	s_replace(s, "Ago", " Ago");
	return s;
}

std::string
HourInDayToLocalizedString(int iHourIndex)
{
	auto iBeginHour = iHourIndex;
	iBeginHour--;
	wrap(iBeginHour, 24);
	iBeginHour++;

	return ssprintf("%02d:00+", iBeginHour);
}

tm
AddDays(tm start, int iDaysToMove)
{
	/*
	 * This causes problems on OS X, which doesn't correctly handle range that
	 * are below their normal values (eg. mday = 0).  According to the manpage,
	 * it should adjust them:
	 *
	 * "If structure members are outside their legal interval, they will be
	 * normalized (so that, e.g., 40 October is changed into 9 November)."
	 *
	 * Instead, it appears to simply fail.
	 *
	 * Refs:
	 *  http://bugs.php.net/bug.php?id=10686
	 *  http://sourceforge.net/tracker/download.php?group_id=37892&atid=421366&file_id=79179&aid=91133
	 *
	 * Note "Log starting 2004-03-07 03:50:42"; mday is 7, and
	 * PrintCaloriesBurned calls us with iDaysToMove = -7, resulting in an
	 * out-of-range value 0.  This seems legal, but OS X chokes on it.
	 */
	/*	start.tm_mday += iDaysToMove;
		time_t seconds = mktime( &start );
		ASSERT( seconds != (time_t)-1 );
		*/

	/* This handles DST differently: it returns the time that was exactly
	 * n*60*60*24 seconds ago, where the above code always returns the same time
	 * of day.  I prefer the above behavior, but I'm not sure that it
	 * mattersmatters. */
	auto seconds = mktime(&start);
	seconds += iDaysToMove * 60 * 60 * 24;

	tm time;
	localtime_r(&seconds, &time);
	return time;
}

tm
GetYesterday(tm start)
{
	return AddDays(start, -1);
}

int
GetDayOfWeek(tm time)
{
	const auto iDayOfWeek = time.tm_wday;
	ASSERT(iDayOfWeek < DAYS_IN_WEEK);
	return iDayOfWeek;
}

tm
GetNextSunday(tm start)
{
	return AddDays(start, DAYS_IN_WEEK - GetDayOfWeek(start));
}

tm
GetDayInYearAndYear(int iDayInYearIndex, int iYear)
{
	/* If iDayInYearIndex is 200, set the date to Jan 200th, and let mktime
	 * round it.  This shouldn't suffer from the OSX mktime() issue described
	 * above, since we're not giving it negative values. */
	tm when;
	ZERO(when);
	when.tm_mon = 0;
	when.tm_mday = iDayInYearIndex + 1;
	when.tm_year = iYear - 1900;
	auto then = mktime(&when);

	localtime_r(&then, &when);
	return when;
}

LuaFunction(MonthToString, MonthToString(Enum::Check<Month>(L, 1)));
LuaFunction(MonthToLocalizedString,
			MonthToLocalizedString(Enum::Check<Month>(L, 1)));
LuaFunction(MonthOfYear, GetLocalTime().tm_mon);
LuaFunction(DayOfMonth, GetLocalTime().tm_mday);
LuaFunction(Hour, GetLocalTime().tm_hour);
LuaFunction(Minute, GetLocalTime().tm_min);
LuaFunction(Second, GetLocalTime().tm_sec);
LuaFunction(Year, GetLocalTime().tm_year + 1900);
LuaFunction(Weekday, GetLocalTime().tm_wday);
LuaFunction(DayOfYear, GetLocalTime().tm_yday);
