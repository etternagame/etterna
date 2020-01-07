#include "Etterna/Globals/global.h"
#include "LowLevelWindow.h"
#include "arch/arch_default.h"

LowLevelWindow*
LowLevelWindow::Create()
{
	return new ARCH_LOW_LEVEL_WINDOW;
}
