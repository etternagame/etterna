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
	string key;
	RString lastsong;
	RString lastpack;
	Difficulty lastdiff = Difficulty_Invalid;
	float rate = 1.f;
	Song* songptr;
	Steps* stepsptr;

	bool IsLoaded() { return loaded; }

	bool loaded = false;
	void FromKey(const string& ck);
	XNode* CreateNode(bool includerate) const;
	void LoadFromNode(const XNode* node);
	void PushSelf(lua_State* L);
};

struct Playlist
{
	RString name;
	vector<Chart> chartlist;
	void Add(Chart ch) { chartlist.emplace_back(ch); }
	void AddChart(const string& ck);
	void SwapPosition();

	void Create();
	vector<vector<string>> courseruns;

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* node);
	int GetNumCharts() { return chartlist.size(); }
	vector<string> GetKeys();
	string GetName() { return name; }
	float GetAverageRating();
	void DeleteChart(int i);

	void PushSelf(lua_State* L);
};

struct CalcTest	{
	string ck;
	float ev;
	float rate;
	map<int, float> version_history;
};

struct CalcTestList
{
	Skillset skillset;
	map<string, CalcTest> filemapping;
	XNode* CreateNode() const;
};

#endif
