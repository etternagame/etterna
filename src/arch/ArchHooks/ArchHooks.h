#ifndef ARCH_HOOKS_H
#define ARCH_HOOKS_H

#include <chrono>
#include <string>

#ifdef __APPLE__
#define ARCH_HOOKS ArchHooks
#elif defined(__unix__)
#define ARCH_HOOKS ArchHooks_Unix
#elif defined(_WIN32)
#define ARCH_HOOKS ArchHooks_Win32
#endif

struct lua_State;
class ArchHooks
{
  public:
	static ArchHooks* Create();
	ArchHooks();
	virtual ~ArchHooks() = default;
	virtual void Init() {}

	// Lua
	void PushSelf(lua_State* L);
	void RegisterWithLua();

  private:
	static bool g_bToggleWindowed;
	bool m_bHasFocus;
	bool m_bFocusChanged;
};

#endif