#include "Etterna/Globals/global.h"
#include "Difficulty.h"
#include "GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "ThemeMetric.h"

static const char* DifficultyNames[] = {
	"Beginner", "Easy", "Medium", "Hard", "Challenge", "Edit",
};
XToString(Difficulty);
StringToX(Difficulty);
LuaXType(Difficulty);

struct OldStyleStringToDifficultyMapHolder
{
	std::map<RString, Difficulty> conversion_map;
	OldStyleStringToDifficultyMapHolder()
	{
		conversion_map["beginner"] = Difficulty_Beginner;
		conversion_map["easy"] = Difficulty_Easy;
		conversion_map["basic"] = Difficulty_Easy;
		conversion_map["light"] = Difficulty_Easy;
		conversion_map["medium"] = Difficulty_Medium;
		conversion_map["another"] = Difficulty_Medium;
		conversion_map["trick"] = Difficulty_Medium;
		conversion_map["standard"] = Difficulty_Medium;
		conversion_map["difficult"] = Difficulty_Medium;
		conversion_map["hard"] = Difficulty_Hard;
		conversion_map["ssr"] = Difficulty_Hard;
		conversion_map["maniac"] = Difficulty_Hard;
		conversion_map["heavy"] = Difficulty_Hard;
		conversion_map["smaniac"] = Difficulty_Challenge;
		conversion_map["challenge"] = Difficulty_Challenge;
		conversion_map["expert"] = Difficulty_Challenge;
		conversion_map["oni"] = Difficulty_Challenge;
		conversion_map["edit"] = Difficulty_Edit;
	}
};
OldStyleStringToDifficultyMapHolder OldStyleStringToDifficulty_converter;
Difficulty
OldStyleStringToDifficulty(const RString& sDC)
{
	RString s2 = sDC;
	s2.MakeLower();
	std::map<RString, Difficulty>::iterator diff =
	  OldStyleStringToDifficulty_converter.conversion_map.find(s2);
	if (diff != OldStyleStringToDifficulty_converter.conversion_map.end()) {
		return diff->second;
	}
	return Difficulty_Invalid;
}

LuaFunction(OldStyleStringToDifficulty, OldStyleStringToDifficulty(SArg(1)));

static ThemeMetric<RString> NAMES("CustomDifficulty", "Names");

RString
GetCustomDifficulty(StepsType st, Difficulty dc)
{
	/* XXX GAMEMAN->GetStepsTypeInfo( StepsType_Invalid ) will crash. I'm not
	 * sure what the correct behavior in this case should be. Should we still
	 * allow custom difficulties? Why do we not allow custom difficulties for
	 * Edit? - Steve */
	// CustomDifficulty for Edit defeats the purpose of the edit's name. -aj
	if (st == StepsType_Invalid) {
		/* This is not totally necessary since DifficultyToString() will
		 * return "", but the comment there says that the caller should
		 * really be checking for invalid values. */
		if (dc == Difficulty_Invalid)
			return RString();
		return DifficultyToString(dc);
	}

	if (dc == Difficulty_Edit) {
		return "Edit";
	}
	// OPTIMIZATION OPPORTUNITY: cache these metrics and cache the splitting
	vector<RString> vsNames;
	split(NAMES, ",", vsNames);
	for(auto sName : vsNames){
		ThemeMetric<StepsType> STEPS_TYPE("CustomDifficulty", sName + "StepsType");
		if (STEPS_TYPE == StepsType_Invalid || st == STEPS_TYPE) // match
		{
			ThemeMetric<Difficulty> DIFFICULTY("CustomDifficulty", sName + "Difficulty");
			if (DIFFICULTY == Difficulty_Invalid || dc == DIFFICULTY) // match
			{
				ThemeMetric<RString> STRING("CustomDifficulty", sName + "String");
				return STRING.GetValue();
			}
		}
	}
	// no matching CustomDifficulty, so use a regular difficulty name
	if (dc == Difficulty_Invalid)
		return RString();
	return DifficultyToString(dc);
}

LuaFunction(GetCustomDifficulty,
			GetCustomDifficulty(Enum::Check<StepsType>(L, 1),
								Enum::Check<Difficulty>(L, 2)));

RString
CustomDifficultyToLocalizedString(const RString& sCustomDifficulty)
{
	return THEME->GetString("CustomDifficulty", sCustomDifficulty);
}

LuaFunction(CustomDifficultyToLocalizedString,
			CustomDifficultyToLocalizedString(SArg(1)));

RString
StepsToCustomDifficulty(const Steps* pSteps)
{
	return GetCustomDifficulty(pSteps->m_StepsType, pSteps->GetDifficulty());
}

#include "Etterna/Models/Lua/LuaBinding.h"

LuaFunction(StepsToCustomDifficulty,
			StepsToCustomDifficulty(Luna<Steps>::check(L, 1)));

/*
 * (c) 2001-2004 Chris Danford
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
