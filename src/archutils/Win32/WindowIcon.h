/* Win32 helper - load an HICON */
#ifndef WINDOW_ICON_H
#define WINDOW_ICON_H

#include <windows.h>
struct RageSurface;

HICON
IconFromSurface(const RageSurface* pImg);
HICON
IconFromFile(const std::string& sIconFile);

#endif
