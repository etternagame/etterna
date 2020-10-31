#ifndef ARCH_HOOKS_WIN32_H
#define ARCH_HOOKS_WIN32_H

#include <windows.h>
#include "ArchHooks.h"
class RageMutex;

class ArchHooks_Win32 : public ArchHooks
{
  public:
	ArchHooks_Win32();
	~ArchHooks_Win32();
	std::string GetArchName() const { return "Windows"; }
	void RestartProgram();
	bool CheckForMultipleInstances(int argc, char* argv[]);

	void BoostPriority();
	void UnBoostPriority();
	void SetupConcurrentRenderingThread();

	std::string GetClipboard();
	/** @brief Fetch the window width. */
	int GetWindowWidth();

	void sShowCursor(bool set) override
	{
		if (set)
			while (ShowCursor(true) < 0)
				;
		else
			while (ShowCursor(false) >= 0)
				;
	}

	/** @brief Fetch the window height. */
	int GetWindowHeight();
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_Win32

#endif
