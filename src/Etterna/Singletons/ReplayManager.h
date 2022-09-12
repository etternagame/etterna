#pragma once
#ifndef REPLAYMAN
#define REPLAYMAN

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/HighScore/Replay.h"

#include <unordered_map>

struct HighScore;
class ReplayManager
{
  public:
	ReplayManager();
	~ReplayManager();

	Replay* GetReplay(HighScore* hs);
	void ReleaseReplay(Replay* replay);

	void PushSelf(lua_State* L);

  private:
	// scorekey to {refcount, pointer}
	std::unordered_map<std::string, std::pair<unsigned, Replay*>>
	  scoresToReplays{};
};

extern std::shared_ptr<ReplayManager> REPLAYS;

#endif
