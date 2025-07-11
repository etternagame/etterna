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
};

#endif
