#include "Etterna/Globals/global.h"
#include "EnumHelper.h"
#include "GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameManager.h"
#include "LocalizedString.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "ThemeMetric.h"

#include <algorithm>

std::string
StepsTypeToString(StepsType st);

extern const std::string RANKING_TO_FILL_IN_MARKER("#P1#");

extern const std::string GROUP_ALL = "---Group All---";

static const char* RadarCategoryNames[] = {
	"Notes", "TapsAndHolds", "Jumps", "Holds", "Mines",
	"Hands", "Rolls",		 "Lifts", "Fakes",
};
XToString(RadarCategory);
XToLocalizedString(RadarCategory);
LuaFunction(RadarCategoryToLocalizedString,
			RadarCategoryToLocalizedString(Enum::Check<RadarCategory>(L, 1)));
LuaXType(RadarCategory);

std::string
StepsTypeToString(StepsType st)
{
	std::string s = GAMEMAN->GetStepsTypeInfo(st).szName; // "dance-single"
	/* foo-bar -> Foo_Bar */
	s_replace(s, "-", "_");

	auto bCapitalizeNextLetter = true;
	for (auto& i : s) {
		if (bCapitalizeNextLetter) {
			i = toupper(i);
			bCapitalizeNextLetter = false;
		}

		if (i == '_')
			bCapitalizeNextLetter = true;
	}

	return s;
}
namespace StringConversion {
template<>
std::string
ToString<StepsType>(const StepsType& value)
{
	return StepsTypeToString(value);
}
}

LuaXType(StepsType);

static const char* PlayerControllerNames[] = {
	"Human",
	"Autoplay",
	"Cpu",
	"Replay",
};
XToString(PlayerController);
StringToX(PlayerController);
XToLocalizedString(PlayerController);
LuaXType(PlayerController);

static const char* HealthStateNames[] = {
	"Hot",
	"Alive",
	"Danger",
	"Dead",
};
XToString(HealthState);
LuaXType(HealthState);

static const char* GameplayModeNames[] = {
	"Normal",
	"Practice",
	"Replay",
};
XToString(GameplayMode);
LuaXType(GameplayMode);

static const char* SortOrderNames[] = {
	"Group",	  "Title",		"BPM",		 "TopGrades", "Artist",
	"Genre",	  "ModeMenu",	"Favorites", "Overall",	  "Stream",
	"Jumpstream", "Handstream", "Stamina",	 "JackSpeed", "Chordjack",
	"Technical",  "Length",		"Ungrouped"
};
XToString(SortOrder);
StringToX(SortOrder);
LuaXType(SortOrder);
XToLocalizedString(SortOrder);
LuaFunction(SortOrderToLocalizedString,
			SortOrderToLocalizedString(Enum::Check<SortOrder>(L, 1)));

static const char* TapNoteScoreNames[] = {
	"None", "HitMine", "AvoidMine", "CheckpointMiss", "Miss", "W5", "W4",
	"W3",	"W2",	   "W1",		"CheckpointHit",
};
struct tns_conversion_helper
{
	std::map<std::string, TapNoteScore> conversion_map;
	tns_conversion_helper()
	{
		FOREACH_ENUM(TapNoteScore, tns)
		{
			conversion_map[TapNoteScoreNames[tns]] = tns;
		}
		// for backward compatibility
		conversion_map["Boo"] = TNS_W5;
		conversion_map["Good"] = TNS_W4;
		conversion_map["Great"] = TNS_W3;
		conversion_map["Perfect"] = TNS_W2;
		conversion_map["Marvelous"] = TNS_W1;
	}
};
tns_conversion_helper tns_converter;
XToString(TapNoteScore);
LuaXType(TapNoteScore);
TapNoteScore
StringToTapNoteScore(const std::string& s)
{
	const auto tns = tns_converter.conversion_map.find(s);
	if (tns != tns_converter.conversion_map.end()) {
		return tns->second;
	}
	return TapNoteScore_Invalid;
}
// This is necessary because the StringToX macro wasn't used, and Preference
// relies on there being a StringConversion entry for enums used in prefs. -Kyz
namespace StringConversion {
template<>
bool
FromString<TapNoteScore>(const std::string& value, TapNoteScore& out)
{
	out = StringToTapNoteScore(value);
	return out != TapNoteScore_Invalid;
}
}
XToLocalizedString(TapNoteScore);
LuaFunction(TapNoteScoreToLocalizedString,
			TapNoteScoreToLocalizedString(Enum::Check<TapNoteScore>(L, 1)));

static const char* HoldNoteScoreNames[] = {
	"None",
	"LetGo",
	"Held",
	"MissedHold",
};
XToString(HoldNoteScore);
LuaXType(HoldNoteScore);
HoldNoteScore
StringToHoldNoteScore(const std::string& s)
{
	// for backward compatibility
	if (s == "NG")
		return HNS_LetGo;
	if (s == "OK")
		return HNS_Held;

	// new style
	if (s == "None")
		return HNS_None;
	if (s == "LetGo")
		return HNS_LetGo;
	if (s == "Held")
		return HNS_Held;
	if (s == "MissedHold")
		return HNS_Missed;

	return HoldNoteScore_Invalid;
}
XToLocalizedString(HoldNoteScore);

static const char* SkillsetNames[] = {
	"Overall", "Stream",	"Jumpstream", "Handstream",
	"Stamina", "JackSpeed", "Chordjack",  "Technical",
};
XToString(Skillset);
LuaXType(Skillset);
Skillset
StringToSkillset(const std::string& s)
{
	if (s == "Overall")
		return Skill_Overall;
	if (s == "Stream")
		return Skill_Stream;
	if (s == "Jumpstream")
		return Skill_Jumpstream;
	if (s == "Handstream")
		return Skill_Jumpstream;
	if (s == "Stamina")
		return Skill_Stamina;
	if (s == "JackSpeed")
		return Skill_JackSpeed;
	if (s == "Chordjack")
		return Skill_Chordjack;
	if (s == "Technical")
		return Skill_Technical;

	return Skill_Overall;
}

static const char* CalcPatternModNames[] = {
	"Stream",
	"JS",
	// "JSS",
	// "JSJ",
	"HS",
	// "HSS",
	// "HSJ",
	"CJ",
	// "CJS",
	// "CJJ",
	"CJDensity",
	"HSDensity",
	"CJOHAnchor",
	"OHJumpMod",
	// "OHJBaseProp",
	// "OHJPropComp",
	// "OHJSeqComp",
	// "OHJMaxSeq",
	// "OHJCCTaps",
	// "OHJHTaps",
	"CJOHJump",
	// "CJOHJPropComp",
	// "CJOHJSeqComp",
	"Balance",
	"Roll",
	"RollJS",
	"OHTrill",
	"VOHTrill",
	"Chaos",
	"FlamJam",
	"WideRangeRoll",
	"WideRangeJumptrill",
	"WideRangeJJ",
	"WideRangeBalance",
	"WideRangeAnchor",
	"TheThing",
	"TheThing2",
	"RanMan",
	"Minijack",
	// "RanLen",
	// "RanAnchLen",
	// "RanAnchLenMod",
	// "RanJack",
	// "RanOHT",
	// "RanOffS",
	// "RanPropAll",
	// "RanPropOff",
	// "RanPropOHT",
	// "RanPropOffS",
	// "RanPropJack",
	"TotalPatternMod",
};
XToString(CalcPatternMod);
LuaXType(CalcPatternMod);

static const char* CalcDiffValueNames[] = { "NPSBase",
											"MSBase",
											"JackBase",
											"CJBase",
											"TechBase",
											"RMABase",
											"MSD" };
XToString(CalcDiffValue);
LuaXType(CalcDiffValue);

static const char* CalcDebugMiscNames[] = { "Pts",
											"PtLoss",
											//"JackPtLoss",
											"StamMod" };
XToString(CalcDebugMisc);
LuaXType(CalcDebugMisc);

static const char* ValidationKeyNames[] = {
	"Brittle",
	"Weak",
};
XToString(ValidationKey);
LuaXType(ValidationKey);
ValidationKey
StringToValidationKey(const std::string& s)
{
	if (s == "Brittle")
		return ValidationKey_Brittle;
	if (s == "Weak")
		return ValidationKey_Weak;
	return ValidationKey_Brittle;
}

static const char* TimingWindowNames[] = { "W1",   "W2",   "W3",
										   "W4",   "W5",   "Mine",
										   "Hold", "Roll", "Checkpoint" };
XToString(TimingWindow);

static const char* ScoreEventNames[] = {
	"CheckpointHit",
	"W1",
	"W2",
	"W3",
	"W4",
	"W5",
	"Miss",
	"HitMine",
	"CheckpointMiss",
	"Held",
	"LetGo",
	"MissedHold",
};
XToString(ScoreEvent);

static const char* TapNoteScoreJudgeTypeNames[] = {
	"MinimumScore",
	"LastScore",
};
XToString(TapNoteScoreJudgeType);
LuaXType(TapNoteScoreJudgeType);

static const char* ProfileSlotNames[] = {
	"Player1",
	"Player2",
};
XToString(ProfileSlot);
LuaXType(ProfileSlot);

void
DisplayBpms::Add(float f)
{
	vfBpms.push_back(f);
}

float
DisplayBpms::GetMin() const
{
	auto fMin = FLT_MAX;
	for (const auto& f : vfBpms) {
		if (f != -1.F)
			fMin = std::min(fMin, f);
	}
	if (fMin == FLT_MAX)
		return 0;
	return fMin;
}

float
DisplayBpms::GetMax() const
{
	return this->GetMaxWithin();
}

float
DisplayBpms::GetMaxWithin(float highest) const
{
	float fMax = 0;
	for (const auto& f : vfBpms) {
		if (f != -1.F)
			fMax = std::clamp(std::max(fMax, f), 0.F, highest);
	}
	return fMax;
}

bool
DisplayBpms::BpmIsConstant() const
{
	return fabsf(GetMin() - GetMax()) < 0.001f;
}

bool
DisplayBpms::IsSecret() const
{
	for (const auto& f : vfBpms) {
		if (f == -1.F)
			return true;
	}
	return false;
}

static const char* StyleTypeNames[] = {
	"OnePlayerOneSide",
	"OnePlayerTwoSides",
};
XToString(StyleType);
StringToX(StyleType);
LuaXType(StyleType);

static const char* SampleMusicPreviewModeNames[] = { "Normal",
													 "StartToPreview",
													 "ScreenMusic",
													 "LastSong",
													 "Nothing" };
XToString(SampleMusicPreviewMode);
StringToX(SampleMusicPreviewMode);
LuaXType(SampleMusicPreviewMode);

static const char* StageNames[] = {
	"1st",	"2nd",	 "3rd",	   "4th",	 "5th",	  "6th",
	"Next", "Final", "Extra1", "Extra2", "Event", "Demo",
};
XToString(Stage);
LuaXType(Stage);
XToLocalizedString(Stage);
LuaFunction(StageToLocalizedString,
			StageToLocalizedString(Enum::Check<Stage>(L, 1)));

static const char* MultiPlayerStatusNames[] = {
	"Joined",
	"NotJoined",
	"Unplugged",
	"MissingMultitap",
};
XToString(MultiPlayerStatus);

static const char* FailTypeNames[] = {
	"Immediate",
	"ImmediateContinue",
	"Off",
};
XToString(FailType);
XToLocalizedString(FailType);
StringToX(FailType);
LuaXType(FailType);
