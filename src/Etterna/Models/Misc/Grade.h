#ifndef GRADE_H
#define GRADE_H

#include "EnumHelper.h"
#include "ThemeMetric.h"

enum Grade
{
	Grade_Tier01, /**< Usually an AAAAA */
	Grade_Tier02, /**< Usually an AAAA++ */
	Grade_Tier03, /**< Usually an AAAA+ */
	Grade_Tier04, /**< Usually an AAAA */
	Grade_Tier05, /**< Usually an AAA++ */
	Grade_Tier06, /**< Usually an AAA+ */
	Grade_Tier07, /**< Usually an AAA */
	Grade_Tier08, /**< Usually an AA++ */
	Grade_Tier09, /**< Usually an AA+ */
	Grade_Tier10, /**< Usually an AA */
	Grade_Tier11, /**< Usually an A++ */
	Grade_Tier12, /**< Usually an A+ */
	Grade_Tier13, /**< Usually an A */
	Grade_Tier14, /**< Usually a B */
	Grade_Tier15, /**< Usually a C */
	Grade_Tier16, /**< Usually a D */
	Grade_Tier17,
	Grade_Tier18,
	Grade_Tier19,
	Grade_Tier20,
	Grade_Failed, /**< Usually an E */
	NUM_Grade,
	Grade_Invalid,
};

/**
 * @brief Convert the grade supplied to a string representation.
 *
 * This is in the header so the test sets don't require Grade.cpp (through
 * PrefsManager), since that pulls in ThemeManager.
 * @param g the grade to convert.
 * @return the string reprsentation.
 */
inline auto
GradeToString(Grade g) -> std::string
{
	ASSERT_M((g >= 0 && g < NUM_Grade) || g == Grade_Invalid,
			 ssprintf("grade = %d", g));

	switch (g) {
		case Grade_Invalid:
			return "NoData";
		case Grade_Failed:
			return "Failed";
		default:
			return ssprintf("Tier%02d", g + 1);
	}
}

/**
 * @brief Convert to the old version styled grade strings.
 *
 * This is mainly for backward compatibility purposes, but the announcer
 * also uses it. Think "AAA", "B", etc.
 * This is only referenced in ScreenEvaluation at the moment.
 * @param g the current Grade.
 * @return the old styled grade string. */
auto
GradeToOldString(Grade g) -> std::string;
auto
GradeToLocalizedString(Grade g) -> std::string;
/**
 * @brief Convert the given std::string into a proper Grade.
 * @param s the string to convert.
 * @return the expected Grade.
 */
auto
StringToGrade(const std::string& s) -> Grade;
LuaDeclareType(Grade);
extern ThemeMetric<int> NUM_GRADE_TIERS_USED;
#define NUM_POSSIBLE_GRADES (NUM_GRADE_TIERS_USED + 1)
/**
 * @brief Step through the enumerator one at a time to get the next Grade.
 * @param g the current Grade.
 * @return the next Grade. */
auto
GetNextPossibleGrade(Grade g) -> Grade;
/** @brief Loop through each possible Grade. */
#define FOREACH_PossibleGrade(g)                                               \
                                                                               \
	for (Grade g = (Grade)(0); (g) != Grade_Invalid;                           \
		 (g) = GetNextPossibleGrade(g))

#endif

auto
GetGradeFromPercent(float pc) -> Grade;
