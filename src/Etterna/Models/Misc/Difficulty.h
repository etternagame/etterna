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
const std::string&
DifficultyToString(Difficulty dc);
Difficulty
StringToDifficulty(const std::string& sDC);
LuaDeclareType(Difficulty);

Difficulty
OldStyleStringToDifficulty(const std::string& sDC); // compatibility

// CustomDifficulty is a themeable difficulty name based on Difficulty, string
// matching on StepsType, and CourseType. It is used to look up localized
// strings and look up colors.
std::string
GetCustomDifficulty(StepsType st, Difficulty dc);
std::string
CustomDifficultyToLocalizedString(const std::string& sCustomDifficulty);
std::string
StepsToCustomDifficulty(const Steps* pSteps);

struct Chart
{
	std::string key;
	std::string lastsong;
	std::string lastpack;
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
	std::string name;
	std::vector<Chart> chartlist;
	void Add(Chart ch) { chartlist.emplace_back(ch); }
	void AddChart(const std::string& ck);
	void SwapPosition();

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

struct CalcTest
{
	std::string ck;
	float ev;
	float rate;
	std::map<int, float> version_history;
};

struct CalcTestList
{
	Skillset skillset;
	std::map<std::string, CalcTest> filemapping;
	XNode* CreateNode() const;
};

#endif
