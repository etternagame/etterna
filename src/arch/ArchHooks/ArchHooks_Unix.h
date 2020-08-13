#ifndef ARCH_HOOKS_UNIX_H
#define ARCH_HOOKS_UNIX_H

#include "ArchHooks.h"
#include <chrono>
class ArchHooks_Unix : public ArchHooks
{
  public:
	void Init();
	std::string GetArchName() const { return "Unix"; }
	void DumpDebugInfo();

	void SetTime(tm newtime);
	int64_t GetMicrosecondsSinceStart();
	std::chrono::microseconds GetChronoDurationSinceStart();

	void MountInitialFilesystems(const std::string& sDirOfExecutable);
	float GetDisplayAspectRatio() { return 4.0f / 3; }

	bool GoToURL(const std::string& sUrl);

	static clockid_t GetClock();

	std::string GetClipboard();
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_Unix

#endif
