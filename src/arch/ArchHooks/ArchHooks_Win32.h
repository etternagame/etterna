#ifndef ARCH_HOOKS_WIN32_H
#define ARCH_HOOKS_WIN32_H

#include <windows.h>
#include "ArchHooks.h"
class RageMutex;

class ArchHooks_Win32 : public ArchHooks
{
  public:
	ArchHooks_Win32();
};

#endif
