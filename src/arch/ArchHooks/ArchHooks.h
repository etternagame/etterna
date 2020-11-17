#ifndef ARCH_HOOKS_H
#define ARCH_HOOKS_H

#include <chrono>
#include <string>

struct lua_State;
class ArchHooks
{
  public:
	static ArchHooks* Create();
	ArchHooks();
	virtual ~ArchHooks() = default;
	virtual void Init() {}

	/* This is called as soon as the loading window is shown, and we can
	 * safely log. */
	virtual void sShowCursor(bool set) {}
	/**
	 * @brief Re-exec the game.
	 *
	 * If this is implemented, it doesn't return. */
	virtual void RestartProgram() {}

	/* If this is a second instance, return true.
	 * Optionally, give focus to the existing window. */
	virtual bool CheckForMultipleInstances(int /* argc */, char*[] /* argv[] */)
	{
		return false;
	}

	/**
	 * @brief Determine if the user wants to quit (eg. ^C, or clicked a "close
	 * window" button).
	 * @return true if the user wants to quit, false otherwise. */
	static bool UserQuit() { return g_bQuitting; }
	static void SetUserQuit() { g_bQuitting = true; }

	/*
	 * Returns true if the user wants to toggle windowed mode and atomically
	 * clears the boolean.
	 */
	static bool GetAndClearToggleWindowed();
	static void SetToggleWindowed();

	/*
	 * Add file search paths, higher priority first.
	 */
	static void MountInitialFilesystems(const std::string& sDirOfExecutable);

	/*
	 * Add file search paths for user-writable directories.
	 */
	static void MountUserFilesystems(const std::string& sDirOfExecutable);

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
	static bool g_bQuitting;
	static bool g_bToggleWindowed;
	bool m_bHasFocus;
	bool m_bFocusChanged;
};

#endif