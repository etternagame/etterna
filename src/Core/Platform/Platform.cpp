#include "Platform.hpp"
#include "Core/Services/Locator.hpp"

/** This file is where functions which have a cross-platform implementation may be defined. */
namespace Core::Platform {

    bool setConsoleEnabled(bool enable) {
    #ifdef _WIN32
        // Disable the console
        if (!enable){
            FreeConsole();
            return true;
        }

        // If we reach this point in the code, attempt to enable the console.
        // Return value of zero means failure to allocate console
        if(AllocConsole() == 0){
            Locator::getLogger()->error("Console window failed to initialize.");
            return false;
        }

        // Usually freopen_s would be used to reassign a file pointer to a new or different file.
        // Since out "file" is standard out, and we only want to redirect it, we can give
        // a dummy value for the file pointer. It can't be null as it is required for the
        // operation to occur, but we don't need to hold onto it afterwards.
        // The following functions should return zero after executing sucessfully. We don't check it here.
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
    #endif
        return true;
    }

    namespace Time {

        /** TODO: Move time related functions to their own class/namespace */
        std::chrono::milliseconds GetChronoDurationSinceStart() {
            return std::chrono::milliseconds(std::chrono::steady_clock::now().time_since_epoch().count() / 1000000);
        }

    } //namespace Time

} // namespace Core::Platform

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
    lua_pushstring(L, Core::Platform::getSystem().c_str());
    return 1;
}

int getClipboard(lua_State* L) {
    lua_pushstring(L, Core::Platform::getClipboard().c_str());
    return 1;
}

int setCursorVisible(lua_State* L){
    Core::Platform::setCursorVisible(BArg(1));
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
