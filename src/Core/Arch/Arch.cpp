#include "Arch.hpp"

/** This file is where functions which have a cross-platform implementation may be defined. */
namespace Core::Arch::Time {

    /** TODO: Move time related functions to their own class/namespace */
    std::chrono::milliseconds GetChronoDurationSinceStart(){
        return std::chrono::milliseconds(std::chrono::steady_clock::now().time_since_epoch().count() / 1000000);
    }

} // namespace Core::Arch::Time

// Lua Link
// TODO: Isolate all Lua code to it's own section of the codebase
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Globals/GameLoop.h"

int isGameFocused(lua_State* L) {
    lua_pushboolean(L, GameLoop::isGameFocused());
    return 1;
}

int getSystem(lua_State* L) {
    lua_pushstring(L, Core::Arch::getSystem().c_str());
    return 1;
}

int getClipboard(lua_State* L) {
    lua_pushstring(L, Core::Arch::getClipboard().c_str());
    return 1;
}

int setCursorVisible(lua_State* L){
    Core::Arch::setCursorVisible(BArg(1));
    return 0;
}

const luaL_Reg ArchTable[] = {
        LIST_METHOD(isGameFocused),
        LIST_METHOD(getSystem),
        LIST_METHOD(getClipboard),
        LIST_METHOD(setCursorVisible),
        { nullptr, nullptr }
};

LUA_REGISTER_NAMESPACE(Arch);