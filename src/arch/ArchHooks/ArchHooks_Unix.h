#ifndef ARCH_HOOKS_UNIX_H
#define ARCH_HOOKS_UNIX_H

#include "ArchHooks.h"
class ArchHooks_Unix : public ArchHooks
{
  public:
	void Init();
	RString GetArchName() const { return "Unix"; }
	void DumpDebugInfo();

	void SetTime(tm newtime);
	int64_t GetMicrosecondsSinceStart();

	void MountInitialFilesystems(const RString& sDirOfExecutable);
	float GetDisplayAspectRatio() { return 4.0f / 3; }

	bool GoToURL(const RString& sUrl);

	static clockid_t GetClock();

	RString GetClipboard();
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_Unix

#endif
