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

/* Making an OpenGL call doesn't also flush the error state; if we happen
 * to have an error from a previous call, then the assert below will fail.
 * Flush it. */
#define FlushGLErrors()                                                        \
	do {                                                                       \
	} while (glGetError() != GL_NO_ERROR)
#define AssertNoGLError()                                                      \
                                                                               \
	{                                                                          \
		GLenum error = glGetError();                                           \
		ASSERT_M(error == GL_NO_ERROR,                                         \
				 RageDisplay_Legacy_Helpers::GLToString(error));               \
	}

#if defined(DEBUG) || !defined(GL_GET_ERROR_IS_SLOW)
#define DebugFlushGLErrors() FlushGLErrors()
#define DebugAssertNoGLError() AssertNoGLError()
#else
#define DebugFlushGLErrors()
#define DebugAssertNoGLError()
#endif

/** @brief Utilities for working with the RageDisplay. */
namespace RageDisplay_Legacy_Helpers {
void
Init();

std::string
GLToString(GLenum e);

/* g_GLPixFmtInfo is used for both texture formats and surface formats.  For
 * example, it's fine to ask for a RagePixelFormat_RGB5 texture, but to supply a
 * surface matching RagePixelFormat_RGB8.  OpenGL will simply discard the extra
 * bits.
 *
 * It's possible for a format to be supported as a texture format but not as a
 * surface format.  For example, if packed pixels aren't supported, we can still
 * use GL_RGB5_A1, but we'll have to convert to a supported surface pixel format
 * first.  It's not ideal, since we'll convert to RGBA8 and OGL will convert
 * back, but it works fine.
 */
struct GLPixFmtInfo_t
{
	GLenum internalfmt; /* target format */
	GLenum format;		/* target format */
	GLenum type;		/* data format */
};

extern const GLPixFmtInfo_t g_GLPixFmtInfo[];
extern RageDisplay::RagePixelFormatDesc PIXEL_FORMAT_DESC[];
};

#endif
