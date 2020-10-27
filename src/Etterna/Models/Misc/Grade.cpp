#include "Etterna/Globals/global.h"
#include "EnumHelper.h"
#include "Grade.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/PrefsManager.h"

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

std::string
GradeToLocalizedString(Grade g)
{
	auto s = GradeToString(g);
	if (!THEME->HasString("Grade", s))
		return "???";
	return THEME->GetString("Grade", s);
}

std::string
GradeToOldString(Grade g)
{
	// string is meant to be human readable
	switch (g) {
		case Grade_Tier01:
		case Grade_Tier02:
		case Grade_Tier03:
		case Grade_Tier04:
			return "AAAA";
		case Grade_Tier05:
		case Grade_Tier06:
		case Grade_Tier07:
			return "AAA";
		case Grade_Tier08:
		case Grade_Tier09:
		case Grade_Tier10:
			return "AA";
		case Grade_Tier11:
		case Grade_Tier12:
		case Grade_Tier13:
			return "A";
		case Grade_Tier14:
			return "B";
		case Grade_Tier15:
			return "C";
		case Grade_Tier16:
			return "D";
		case Grade_Failed:
			return "E";
		case Grade_Invalid:
			return "N";
		default:
			return "N";
	}
};

Grade
StringToGrade(const std::string& sGrade)
{
	auto s = make_upper(sGrade);

	// new style
	int iTier;
	if (sscanf(sGrade.c_str(), "Tier%02d", &iTier) == 1 && iTier >= 0 &&
		iTier < NUM_Grade)
		return static_cast<Grade>(iTier - 1);
	else if (s == "FAILED")
		return Grade_Failed;
	else if (s == "NODATA")
		return Grade_Invalid;

	Locator::getLogger()->warn("Invalid grade: {}", sGrade.c_str());
	return Grade_Invalid;
};

// get appropriated (for when we have scores but no highscore object to get
// wifegrades) -mina
Grade
GetGradeFromPercent(float pc)
{
	if (pc >= 0.99996F) {
		return Grade_Tier01;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.9998F) {
		return Grade_Tier02;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.9997F) {
		return Grade_Tier03;
	}
	if (pc >= 0.99955F) {
		return Grade_Tier04;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.999F) {
		return Grade_Tier05;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.998F) {
		return Grade_Tier06;
	}
	if (pc >= 0.997F) {
		return Grade_Tier07;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.99F) {
		return Grade_Tier08;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.965F) {
		return Grade_Tier09;
	}
	if (pc >= 0.93F) {
		return Grade_Tier10;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.9F) {
		return Grade_Tier11;
	}
	if (PREFSMAN->m_bUseMidGrades && pc >= 0.85F) {
		return Grade_Tier12;
	}
	if (pc >= 0.8F) {
		return Grade_Tier13;
	}
	if (pc >= 0.7F) {
		return Grade_Tier14;
	}
	if (pc >= 0.6F) {
		return Grade_Tier15;
	}

	return Grade_Tier16;
}
