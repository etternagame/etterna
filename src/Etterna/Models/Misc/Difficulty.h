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
auto
DifficultyToString(Difficulty dc) -> const std::string&;
auto
StringToDifficulty(const std::string& sDC) -> Difficulty;
LuaDeclareType(Difficulty);

auto
OldStyleStringToDifficulty(const std::string& sDC)
  -> Difficulty; // compatibility

// CustomDifficulty is a themeable difficulty name based on Difficulty, string
// matching on StepsType, and CourseType. It is used to look up localized
// strings and look up colors.
auto
GetCustomDifficulty(StepsType st, Difficulty dc) -> std::string;
auto
CustomDifficultyToLocalizedString(const std::string& sCustomDifficulty)
  -> std::string;
auto
StepsToCustomDifficulty(const Steps* pSteps) -> std::string;

struct Chart
{
	std::string key;
	std::string lastsong;
	std::string lastpack;
	Difficulty lastdiff = Difficulty_Invalid;
	float rate = 1.F;
	Song* songptr{};
	Steps* stepsptr{};

	auto IsLoaded() -> bool { return loaded; }

	bool loaded = false;
	void FromKey(const std::string& ck);
	[[nodiscard]] auto CreateNode(bool includerate) const -> XNode*;
	void LoadFromNode(const XNode* node);
	void PushSelf(lua_State* L);
};

struct Playlist
{
	std::string name;
	std::vector<Chart> chartlist;
	void Add(const Chart& ch) { chartlist.emplace_back(ch); }
	void AddChart(const std::string& ck);
	void SwapPosition();

	std::vector<std::vector<std::string>> courseruns;

	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* node);
	auto GetNumCharts() -> size_t { return chartlist.size(); }
	auto GetKeys() -> std::vector<std::string>;
	auto GetName() -> std::string { return name; }
	auto GetAverageRating() -> float;
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
	[[nodiscard]] auto CreateNode() const -> XNode*;
};

#endif
