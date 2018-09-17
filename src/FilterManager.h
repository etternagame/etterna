#ifndef FilterManager_H
#define FilterManager_H

#include "global.h"
#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"
#include <unordered_map>

class PlayerState;
class FilterManager
{
  public:
	FilterManager();
	~FilterManager();

	PlayerState* m_pPlayerState[NUM_PLAYERS];

	float SSFilterLowerBounds[NUM_Skillset + 1];
	float SSFilterUpperBounds[NUM_Skillset + 1];
	float MaxFilterRate = 1.f;
	bool ExclusiveFilter = false; // if true the filter system will only match
								  // songs that meet all criteria rather than
								  // all that meet any - mina
	float GetSSFilter(Skillset ss, int bound);
	void SetSSFilter(float v, Skillset ss, int bound);
	void ResetSSFilters(); // reset button for filters
	bool HighestSkillsetsOnly = false;
	bool AnyActiveFilter();

	// not actually filter stuff! but this doesn't get enough love so i'm going
	// to put it here until i make something for it -mina
	int miniboarddockx = 0;
	int miniboarddocky = 0;
	bool galaxycollapsed = false;
	unordered_map<string, pair<int, int>> watte;

	// Lua
	void PushSelf(lua_State* L);
};

extern FilterManager* FILTERMAN;

#endif