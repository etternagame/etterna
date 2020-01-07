#ifndef ARCH_HOOKS_MACOSX_H
#define ARCH_HOOKS_MACOSX_H

#include "ArchHooks.h"

class ArchHooks_MacOSX : public ArchHooks
{
  public:
	void Init();
	RString GetArchName() const;
	void DumpDebugInfo();
	RString GetPreferredLanguage();
	bool GoToURL(const RString& sUrl);
	float GetDisplayAspectRatio();
};

#ifdef ARCH_HOOKS
#error "More than one ArchHooks selected!"
#endif
#define ARCH_HOOKS ArchHooks_MacOSX

#endif /* ARCH_HOOKS_MACOSX_H */
