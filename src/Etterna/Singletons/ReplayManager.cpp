#include "Etterna/Globals/global.h"
#include "ReplayManager.h"

#include <memory>

std::shared_ptr<ReplayManager> REPLAYS = nullptr;

std::shared_ptr<Replay> dummyReplay = std::make_shared<Replay>();

std::shared_ptr<Replay>
ReplayManager::GetReplay(HighScore* hs) {
	if (hs == nullptr) {
		return dummyReplay;
	}
	return std::make_shared<Replay>(hs);
}

ReplayManager::ReplayManager() {
	// Register with Lua.
	Lua* L = LUA->Get();
	lua_pushstring(L, "REPLAYS");
	this->PushSelf(L);
	lua_settable(L, LUA_GLOBALSINDEX);
	LUA->Release(L);
}

ReplayManager::~ReplayManager() {
	// under normal conditions LUA is null
	// but if this is triggered otherwise, this should happen
	if (LUA != nullptr) {
		// Unregister with Lua.
		LUA->UnsetGlobal("REPLAYS");
	}
}

#include "Etterna/Models/Lua/LuaBinding.h"
class LunaReplayManager : public Luna<ReplayManager>
{
  public:
	LunaReplayManager() {
	}
};
LUA_REGISTER_CLASS(ReplayManager)
