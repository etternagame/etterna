#ifndef RAGE_DISPLAY_OGL_HELPERS_H
#define RAGE_DISPLAY_OGL_HELPERS_H

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glew.h>

/* Import RageDisplay, for types.  Do not include RageDisplay_Legacy.h. */
#include "RageUtil/Graphics/RageDisplay.h"

/* Windows defines GL_EXT_paletted_texture incompletely: */
#ifndef GL_TEXTURE_INDEX_SIZE_EXT
#define GL_TEXTURE_INDEX_SIZE_EXT 0x80ED
#endif

/** @brief Utilities for working with the RageDisplay. */
namespace RageDisplay_Legacy_Helpers {
void
Init();
std::string
GLToString(GLenum e);
};

#endif
