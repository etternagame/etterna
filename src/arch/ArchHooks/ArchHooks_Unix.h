#ifndef ARCH_HOOKS_UNIX_H
#define ARCH_HOOKS_UNIX_H

#include "ArchHooks.h"
#include <chrono>
class ArchHooks_Unix : public ArchHooks
{
  public:
	void Init();
	int64_t GetMicrosecondsSinceStart();
	std::chrono::microseconds GetChronoDurationSinceStart();

	void MountInitialFilesystems(const std::string& sDirOfExecutable);
	static clockid_t GetClock();

	std::string GetClipboard();
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_Unix

#endif
