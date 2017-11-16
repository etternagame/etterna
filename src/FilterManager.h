#ifndef FilterManager_H
#define FilterManager_H
#include "PlayerNumber.h"
#include "GameConstantsAndTypes.h"

class PlayerState;
class FilterManager {
public:
	FilterManager();
	~FilterManager();

	PlayerState* m_pPlayerState[NUM_PLAYERS];

	float SSFilterLowerBounds[NUM_Skillset+1];
	float SSFilterUpperBounds[NUM_Skillset+1];
	float MaxFilterRate = 1.f;
	bool ExclusiveFilter = false;	// if true the filter system will only match songs that meet all criteria rather than all that meet any - mina
	float GetSSFilter(Skillset ss, int bound);
	void SetSSFilter(float v, Skillset ss, int bound);
	void ResetSSFilters(); // reset button for filters
	bool HighestSkillsetsOnly = false;
	bool AnyActiveFilter();

	//Lua
	void PushSelf(lua_State *L);
};

extern FilterManager* FILTERMAN;

#endif