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
	/*
	 * Return the general name of the architecture, eg. "Windows", "OS X",
	 * "Unix".
	 */
	virtual std::string GetArchName() const { return "generic"; }

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

	virtual void BoostPriority() {}
	virtual void UnBoostPriority() {}

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
	 * Return the amount of time since the program started.  (This may actually
	 * be since the initialization of HOOKS.
	 *
	 * This is a static function, implemented in whichever ArchHooks source is
	 * used, so it can be used at any time (such as in global constructors),
	 * before HOOKS is initialized.
	 *
	 * RageTimer layers on top of this.
	 */
	static int64_t GetMicrosecondsSinceStart();
	static std::chrono::microseconds GetChronoDurationSinceStart();

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

	/** @brief Fetch the contents of the system clipboard. */
	virtual std::string GetClipboard();

	/** @brief Fetch the window width. */
	virtual int GetWindowWidth();

	/** @brief Fetch the window height. */
	virtual int GetWindowHeight();

	// Lua
	void PushSelf(lua_State* L);
	void RegisterWithLua();

  private:
	/* This are helpers for GetMicrosecondsSinceStart on systems with a timer
	 * that may loop or move backwards. */
	static int64_t FixupTimeIfLooped(int64_t usecs);
	static int64_t FixupTimeIfBackwards(int64_t usecs);

	static bool g_bQuitting;
	static bool g_bToggleWindowed;
	bool m_bHasFocus;
	bool m_bFocusChanged;
};

#endif