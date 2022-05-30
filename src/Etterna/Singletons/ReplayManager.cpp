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

#include "Etterna/Models/Lua/LuaBinding.h"
class LunaReplayManager : public Luna<ReplayManager>
{
  public:
	LunaReplayManager() {
	}
};
LUA_REGISTER_CLASS(ReplayManager)
