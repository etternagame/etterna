#pragma once
#ifndef REPLAYMAN
#define REPLAYMAN

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/HighScore/Replay.h"

struct HighScore;
class ReplayManager
{
  public:
	std::shared_ptr<Replay> GetReplay(HighScore* hs);

	void PushSelf(lua_State* L);

  private:
	std::unordered_map<std::string, std::shared_ptr<Replay>> scoresToReplays;
};

extern std::shared_ptr<ReplayManager> REPLAYS;

#endif
