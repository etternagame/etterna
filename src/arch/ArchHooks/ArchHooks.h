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

	/*
	 * Returns true if the user wants to toggle windowed mode and atomically
	 * clears the boolean.
	 */
	static bool GetAndClearToggleWindowed();
	static void SetToggleWindowed();

	/*
	 * Platform-specific code calls this to indicate focus changes.
	 */
	void SetHasFocus(bool bAppHasFocus);

	/*
	 * Return true if the application has input focus.
	 */
	bool AppHasFocus() const { return m_bHasFocus; }

	/*
	 * Returns true if the application's focus has changed since last called.
	 */
	bool AppFocusChanged();

	// Lua
	void PushSelf(lua_State* L);
	void RegisterWithLua();

  private:
	static bool g_bToggleWindowed;
	bool m_bHasFocus;
	bool m_bFocusChanged;
};

#endif