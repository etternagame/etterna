﻿#include "Etterna/Globals/global.h"
#include "EnumHelper.h"
#include "Grade.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"

LuaXType(Grade);

/** @brief The current number of grade tiers being used. */
ThemeMetric<int> NUM_GRADE_TIERS_USED("PlayerStageStats", "NumGradeTiersUsed");

Grade
GetNextPossibleGrade(Grade g)
{
	if (g < NUM_GRADE_TIERS_USED - 1)
		return static_cast<Grade>(g + 1);
	else if (g == NUM_GRADE_TIERS_USED - 1)
		return Grade_Failed;
	else
		return Grade_Invalid;
}

RString
GradeToLocalizedString(Grade g)
{
	RString s = GradeToString(g);
	if (!THEME->HasString("Grade", s))
		return "???";
	return THEME->GetString("Grade", s);
}

RString
GradeToOldString(Grade g)
{
	// string is meant to be human readable
	switch (g) {
		case Grade_Tier01:
			return "AAAA";
		case Grade_Tier02:
			return "AAA";
		case Grade_Tier03:
			return "AA";
		case Grade_Tier04:
			return "A";
		case Grade_Tier05:
			return "B";
		case Grade_Tier06:
			return "C";
		case Grade_Tier07:
			return "D";
		case Grade_Failed:
			return "E";
		case Grade_NoData:
			return "N";
		default:
			return "N";
	}
};

Grade
StringToGrade(const RString& sGrade)
{
	RString s = sGrade;
	s.MakeUpper();

	// new style
	int iTier;
	if (sscanf(sGrade.c_str(), "Tier%02d", &iTier) == 1 && iTier >= 0 &&
		iTier < NUM_Grade)
		return static_cast<Grade>(iTier - 1);
	else if (s == "FAILED")
		return Grade_Failed;
	else if (s == "NODATA")
		return Grade_NoData;

	LOG->Warn("Invalid grade: %s", sGrade.c_str());
	return Grade_NoData;
};
