#if defined(ANDROID)
#include "Android/SpecialDirs.h"
#elif defined(_WIN32)
#include "Win32/SpecialDirs.h"
#elif defined(__APPLE__)
#include "Darwin/SpecialDirs.h"
#elif defined(__unix__)
#include "Unix/SpecialDirs.h"
#endif
