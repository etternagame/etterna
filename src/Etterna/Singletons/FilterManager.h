#ifndef FilterManager_H
#define FilterManager_H
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include <unordered_map>
#include <array>

class PlayerState;

namespace FilterManagerDefault
{
	inline constexpr float MinFilterRate = 1.F;
	inline constexpr float MaxFilterRate = 1.F;
	inline constexpr bool ExclusiveFilter = false;
	inline constexpr bool HighestSkillsetsOnly = false;
	inline constexpr bool HighestDifficultyOnly = false;
}

class FilterManager
{
  public:
	FilterManager();
	~FilterManager();

	// The skillsets, plus 2 more for Length and Clear%
	constexpr static auto NUM_FILTERS = NUM_Skillset + 2;

	std::array<float, NUM_FILTERS> FilterLowerBounds;
	std::array<float, NUM_FILTERS> FilterUpperBounds;
	/* Skill_Overall,
	 * Skill_Stream,
	 * Skill_Jumpstream,
	 * Skill_Handstream,
	 * Skill_Stamina,
	 * Skill_JackSpeed,
	 * Skill_Chordjack,
	 * Skill_Technical,
	 * Length, //REQUIRED in non-exclusive filter if set
	 * Clear % //REQUIRED in non-exclusive filter if set
	 */
	float MaxFilterRate = FilterManagerDefault::MaxFilterRate;
	float MinFilterRate = FilterManagerDefault::MinFilterRate;
	bool ExclusiveFilter = FilterManagerDefault::ExclusiveFilter; // if true the filter system will only match
								  // songs that meet all criteria rather than
								  // all that meet any - mina
	auto GetFilter(Skillset ss, int bound) -> float;
	void SetFilter(float v, Skillset ss, int bound);
	void ResetSSFilters(); // reset button for filters
	void ResetAllFilters();
	bool HighestSkillsetsOnly = FilterManagerDefault::HighestSkillsetsOnly;
	// Skillset is highest of the chart's skillset
	bool HighestDifficultyOnly = FilterManagerDefault::HighestDifficultyOnly;
	// Chart's skillset's MSD is the highest of all the MSDS of that
	// skillset for all charts for that song.

	auto AnyActiveFilter() -> bool;

	void savepos(std::string name, int x, int y);
	auto loadpos(std::string name) -> std::pair<int, int>;

	// not actually filter stuff! but this doesn't get enough love so i'm going
	// to put it here until i make something for it -mina
	int miniboarddockx = 0;
	int miniboarddocky = 0;
	bool galaxycollapsed = false;
	std::unordered_map<std::string, std::pair<int, int>> watte;

	// General boolean to see if we should be filtering common packs.
	// It defaults to on just to help smooth the multiplayer experience. -poco
	bool filteringCommonPacks = true;

	// Lua
	void PushSelf(lua_State* L);
};

extern FilterManager* FILTERMAN;

#endif
