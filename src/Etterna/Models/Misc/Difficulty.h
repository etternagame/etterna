#ifndef DIFFICULTY_H
#define DIFFICULTY_H

#include "EnumHelper.h"
#include "GameConstantsAndTypes.h"
class Song;
class Steps;

// Player number stuff
enum Difficulty
{
	Difficulty_Beginner,
	Difficulty_Easy,
	Difficulty_Medium,
	Difficulty_Hard,
	Difficulty_Challenge,
	Difficulty_Edit,
	NUM_Difficulty,
	Difficulty_Invalid
};
const RString&
DifficultyToString(Difficulty dc);
Difficulty
StringToDifficulty(const RString& sDC);
LuaDeclareType(Difficulty);

Difficulty
OldStyleStringToDifficulty(const RString& sDC); // compatibility

// CustomDifficulty is a themeable difficulty name based on Difficulty, string
// matching on StepsType, and CourseType. It is used to look up localized
// strings and look up colors.
RString
GetCustomDifficulty(StepsType st, Difficulty dc);
RString
CustomDifficultyToLocalizedString(const RString& sCustomDifficulty);
RString
StepsToCustomDifficulty(const Steps* pSteps);

struct Chart
{
	std::string key;
	RString lastsong;
	RString lastpack;
	Difficulty lastdiff = Difficulty_Invalid;
	float rate = 1.f;
	Song* songptr;
	Steps* stepsptr;

	bool IsLoaded() { return loaded; }

	bool loaded = false;
	void FromKey(const std::string& ck);
	XNode* CreateNode(bool includerate) const;
	void LoadFromNode(const XNode* node);
	void PushSelf(lua_State* L);
};

struct Playlist
{
	RString name;
	std::vector<Chart> chartlist;
	void Add(Chart ch) { chartlist.emplace_back(ch); }
	void AddChart(const std::string& ck);
	void SwapPosition();

	void Create();
	std::vector<std::vector<std::string>> courseruns;

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* node);
	int GetNumCharts() { return chartlist.size(); }
	std::vector<std::string> GetKeys();
	std::string GetName() { return name; }
	float GetAverageRating();
	void DeleteChart(int i);

	void PushSelf(lua_State* L);
};

#endif

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
