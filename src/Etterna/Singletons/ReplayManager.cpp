#include "Etterna/Globals/global.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "ReplayManager.h"

#include <memory>

std::shared_ptr<ReplayManager> REPLAYS = nullptr;

Replay* dummyReplay = new Replay;

Replay*
ReplayManager::GetReplay(HighScore* hs) {
	if (hs == nullptr) {
		return dummyReplay;
	}
	const auto key = hs->GetScoreKey();
	auto it = scoresToReplays.find(key);
	if (it == scoresToReplays.end()) {
		Replay* replay = new Replay(hs);
		std::pair<unsigned, Replay*> value(1, replay);
		scoresToReplays.emplace(key, value);
		return replay;
	} else {
		it->second.first++;
		return it->second.second;
	}
}

void
ReplayManager::ReleaseReplay(Replay* replay) {
	if (replay == nullptr) {
		return;
	}
	const auto key = replay->GetScoreKey();
	auto it = scoresToReplays.find(key);
	if (it == scoresToReplays.end()) {
		Locator::getLogger()->fatal(
		  "Tried to free replay {} with no existing refs! Programming error!",
		  key);
	} else {
		it->second.first--;
		if (it->second.first <= 0) {
			Locator::getLogger()->trace("Freeing replay {}", key);
			delete it->second.second;
			scoresToReplays.erase(it);
		}
	}
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
