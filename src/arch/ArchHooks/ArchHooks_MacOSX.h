#ifndef ARCH_HOOKS_MACOSX_H
#define ARCH_HOOKS_MACOSX_H

#include "ArchHooks.h"

class ArchHooks_MacOSX : public ArchHooks
{
  public:
	void Init();
	std::string GetArchName() const;
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_MacOSX

#endif /* ARCH_HOOKS_MACOSX_H */
