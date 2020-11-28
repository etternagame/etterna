#include "GLHelper.hpp"

/** @brief Clear any errors in the opengl error queue */
void GLClearError(){
    while(glGetError() != GL_NO_ERROR);
}

/**
 * Write a log message if any errors are found in the GL error queue
 * @param function Passed in as macro where function is called
 * @param file Passed in as __FILE__ macro
 * @param line Passed in as __LINE__ macro
 * @return True if there is an error, false if there is no error
 */
bool GLLogCall(const char* function, const char* file, int line){
    while(GLenum error = glGetError()){
        std::cout << "[OpenGL Error] (" << error << "): " << function << " " << file << ":" << line << "\n";
        return false;
    }
    return true;
}

