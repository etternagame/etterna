#include <glad/glad.h>
#include <iostream>

#ifndef RAGEUTIL_GRAPHICS_DISPLAY_OPENGL_GLHELPER_HPP
#define RAGEUTIL_GRAPHICS_DISPLAY_OPENGL_GLHELPER_HPP

#define ASSERT(x)  if(!(x)) abort();
#define GLCall(x) GLClearError(); \
    x;                           \
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);

#endif //RAGEUTIL_GRAPHICS_DISPLAY_OPENGL_GLHELPER_HPP
