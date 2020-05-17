#include "Etterna/Globals/global.h"
#include "EnumHelper.h"
#include "Foreach.h"
#include "GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "LocalizedString.h"
#include "Etterna/Singletons/LuaManager.h"
#include "PlayerNumber.h"
#include "RageUtil/Utils/RageUtil.h"
#include "ThemeMetric.h"

RString
StepsTypeToString(StepsType st);

// This was formerly used to fill in RANKING_TO_FILL_IN_MARKER when it was a
// vector of RStrings. -poco
static vector<RString>
GenerateRankingToFillInMarker()
{
	vector<RString> vRankings;
	vRankings.push_back(ssprintf("#P%d#", PLAYER_1 + 1));
	return vRankings;
}
extern const RString RANKING_TO_FILL_IN_MARKER("#P1#");

extern const RString GROUP_ALL = "---Group All---";

static const char* RadarCategoryNames[] = {
	"Notes", "TapsAndHolds", "Jumps", "Holds", "Mines",
	"Hands", "Rolls",		 "Lifts", "Fakes",
};
XToString(RadarCategory);
XToLocalizedString(RadarCategory);
LuaFunction(RadarCategoryToLocalizedString,
			RadarCategoryToLocalizedString(Enum::Check<RadarCategory>(L, 1)));
LuaXType(RadarCategory);

RString
StepsTypeToString(StepsType st)
{
	RString s = GAMEMAN->GetStepsTypeInfo(st).szName; // "dance-single"
	/* foo-bar -> Foo_Bar */
	s.Replace('-', '_');

	bool bCapitalizeNextLetter = true;
	for (int i = 0; i < static_cast<int>(s.length()); i++) {
		if (bCapitalizeNextLetter) {
			s[i] = toupper(s[i]);
			bCapitalizeNextLetter = false;
		}

		if (s[i] == '_')
			bCapitalizeNextLetter = true;
	}

	return s;
}
namespace StringConversion {
template<>
RString
ToString<StepsType>(const StepsType& value)
{
	return StepsTypeToString(value);
}
}

LuaXType(StepsType);

static const char* PlayModeNames[] = { "Regular" };
XToString(PlayMode);
XToLocalizedString(PlayMode);
StringToX(PlayMode);
LuaFunction(PlayModeToLocalizedString,
			PlayModeToLocalizedString(Enum::Check<PlayMode>(L, 1)));
LuaXType(PlayMode);

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
	"Preferred", "Group",	 "Title",	 "BPM",		 "Popularity",
	"TopGrades", "Artist",	"Genre",	 "ModeMenu",   "Recent",
	"Favorites", "Overall",   "Stream",	"Jumpstream", "Handstream",
	"Stamina",   "JackSpeed", "Chordjack", "Technical",  "Length"
};
XToString(SortOrder);
StringToX(SortOrder);
LuaXType(SortOrder);
XToLocalizedString(SortOrder);
LuaFunction(SortOrderToLocalizedString,
			SortOrderToLocalizedString(Enum::Check<SortOrder>(L, 1)));

static const char* TapNoteScoreNames[] = {
	"None", "HitMine", "AvoidMine", "CheckpointMiss", "Miss", "W5", "W4",
	"W3",   "W2",	  "W1",		"CheckpointHit",
};
struct tns_conversion_helper
{
	std::map<RString, TapNoteScore> conversion_map;
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
StringToTapNoteScore(const RString& s)
{
	std::map<RString, TapNoteScore>::iterator tns =
	  tns_converter.conversion_map.find(s);
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
FromString<TapNoteScore>(const RString& value, TapNoteScore& out)
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
StringToHoldNoteScore(const RString& s)
{
	// for backward compatibility
	if (s == "NG")
		return HNS_LetGo;
	else if (s == "OK")
		return HNS_Held;

	// new style
	else if (s == "None")
		return HNS_None;
	else if (s == "LetGo")
		return HNS_LetGo;
	else if (s == "Held")
		return HNS_Held;
	else if (s == "MissedHold")
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
StringToSkillset(const RString& s)
{
	if (s == "Overall")
		return Skill_Overall;
	else if (s == "Stream")
		return Skill_Stream;
	else if (s == "Jumpstream")
		return Skill_Jumpstream;
	else if (s == "Handstream")
		return Skill_Jumpstream;
	else if (s == "Stamina")
		return Skill_Stamina;
	else if (s == "JackSpeed")
		return Skill_JackSpeed;
	else if (s == "Chordjack")
		return Skill_Chordjack;
	else if (s == "Technical")
		return Skill_Technical;

	return Skill_Overall;
}

static const char* CalcPatternModNames[] = { "OHJump",
											 "Anchor",
											 "Roll",
											 "HS",
											 "HSS",
											 "HSJ",
											 "JS", "JSS", "JSJ",  "CJ",	"CJS", "CJJ", "StreamMod", "OHTrill", "Chaos" , "FlamJam", "WideRangeRoll", "WideRangeJumptrill", "WideRangeBalance", "WideRangeAnchor", "CJOHJump", "CJQuad", "TheThing"};
XToString(CalcPatternMod);
LuaXType(CalcPatternMod);

static const char* CalcDiffValueNames[] = {
	"BaseNPS", "BaseMS", "BaseMSD", "MSD",
};
XToString(CalcDiffValue);
LuaXType(CalcDiffValue);

static const char* CalcDebugMiscNames[] = { "PtLoss",
											"JackPtLoss",
											"StamMod",
											"JackStamMod" };
XToString(CalcDebugMisc);
LuaXType(CalcDebugMisc);

static const char* ValidationKeyNames[] = {
	"Brittle",
	"Weak",
};
XToString(ValidationKey);
LuaXType(ValidationKey);
ValidationKey
StringToValidationKey(const RString& s)
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

static const char* StageAwardNames[] = {
	"FullComboW3",   "SingleDigitW3", "OneW3",		 "FullComboW2",
	"SingleDigitW2", "OneW2",		  "FullComboW1", "80PercentW3",
	"90PercentW3",   "100PercentW3",
};

void
DisplayBpms::Add(float f)
{
	vfBpms.push_back(f);
}

float
DisplayBpms::GetMin() const
{
	float fMin = FLT_MAX;
	FOREACH_CONST(float, vfBpms, f)
	{
		if (*f != -1)
			fMin = min(fMin, *f);
	}
	if (fMin == FLT_MAX)
		return 0;
	else
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
	FOREACH_CONST(float, vfBpms, f)
	{
		if (*f != -1)
			fMax = clamp(max(fMax, *f), 0, highest);
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
	FOREACH_CONST(float, vfBpms, f)
	{
		if (*f == -1)
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
													 "LastSong" };
XToString(SampleMusicPreviewMode);
StringToX(SampleMusicPreviewMode);
LuaXType(SampleMusicPreviewMode);

static const char* StageNames[] = {
	"1st",  "2nd",   "3rd",	"4th",	"5th",   "6th",
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
