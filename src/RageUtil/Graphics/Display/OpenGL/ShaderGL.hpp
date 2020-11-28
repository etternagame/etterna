#ifndef RAGEUTIL_GRAPHICS_OPENGL_SHADERGL_HPP
#define RAGEUTIL_GRAPHICS_OPENGL_SHADERGL_HPP

#include <ghc/filesystem.hpp>

#include <string>
#include <vector>

/**
 * Manages a single shader within OpenGL
 */
class ShaderGL {
public:
    ShaderGL() : programID(0) {}
    ShaderGL(unsigned int type, const ghc::filesystem::path& path);
    ShaderGL(const ghc::filesystem::path &vertexPath, const ghc::filesystem::path &fragmentPath);
    ~ShaderGL();

    // Modifying shader
    void addVertexShader(const ghc::filesystem::path& vertexPath);
    void addFragmentShader(const ghc::filesystem::path& fragmentPath);
    void compile();

    // Binding
    void bind() const;
    static void unbind();

    // Uniform
    void setUniform1i(const std::string& name, int value);

    // Compilation
    static unsigned int compileShader(unsigned int type, const ghc::filesystem::path &path);
    bool isActive();
private:
    // Fields
    unsigned int programID;
    std::vector<unsigned int> shaderIDs;

	int getUniformLocation(const std::string& name) const;
};


#endif //RAGEUTIL_GRAPHICS_OPENGL_SHADERGL_HPP
