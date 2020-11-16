#ifndef ARCH_HOOKS_UNIX_H
#define ARCH_HOOKS_UNIX_H

#include "ArchHooks.h"
class ArchHooks_Unix : public ArchHooks
{
  public:
	void Init();

	void MountInitialFilesystems(const std::string& sDirOfExecutable);
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_Unix

#endif
