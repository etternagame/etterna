#include "ShaderGL.hpp"

#include "GLHelper.hpp"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"

#include <fmt/format.h>
#include <glad/glad.h>

#include <vector>
#include <stdexcept>

/**
 * A shader constructor incase there is only a single shader type.
 * Convenience ctor to help with refactoring RageDisplay_OGL
 * @param type Shader type
 * @param path Shader path relative to app directory
 */
ShaderGL::ShaderGL(unsigned int type, const ghc::filesystem::path &path) : programID(0) {
    if(type == GL_VERTEX_SHADER) this->addVertexShader(path);
    if(type == GL_FRAGMENT_SHADER) this->addFragmentShader(path);
    this->compile();
}

ShaderGL::ShaderGL(const ghc::filesystem::path &vertexPath, const ghc::filesystem::path &fragmentPath) : programID(0) {
    this->addVertexShader(vertexPath);
    this->addFragmentShader(fragmentPath);
    this->compile();
}

ShaderGL::~ShaderGL() {
    glDeleteProgram(this->programID);
}

void ShaderGL::addVertexShader(const ghc::filesystem::path &vertexPath) {
    unsigned vertexID = ShaderGL::compileShader(GL_VERTEX_SHADER, vertexPath);
    this->shaderIDs.push_back(vertexID);
}

void ShaderGL::addFragmentShader(const ghc::filesystem::path &fragmentPath) {
    unsigned fragmentID = ShaderGL::compileShader(GL_FRAGMENT_SHADER, fragmentPath);
    this->shaderIDs.push_back(fragmentID);
}

void ShaderGL::compile() {
    this->programID = glCreateProgram();
    for(auto const& id : this->shaderIDs){
        glAttachShader(programID, id);
    }
	glLinkProgram(programID);

	// Check compile status
	int status;
	glGetProgramiv(this->programID, GL_LINK_STATUS, &status);
	if(status == GL_TRUE)
		return;

	// Print error information
	int errorMaxLength = 2048;
	char programLog[2048];
	glGetProgramInfoLog(this->programID, errorMaxLength, nullptr, programLog);
	throw std::runtime_error(fmt::format("Shader failed to link: {}", programLog));
}

void ShaderGL::bind() const {
	glUseProgram(this->programID);
}

void ShaderGL::unbind() {
    glUseProgram(0);
}

void ShaderGL::setUniform1i(const std::string &name, int value) {
    glUniform1i(this->getUniformLocation(name), value);
}

int ShaderGL::getUniformLocation(const std::string &name) const {
   	int location = glGetUniformLocation(programID, name.data());
    if(location == -1)
        std::cout << "Warning: Uniform \"" << name << "\" does not exist.\n";
    return location;
}

unsigned int ShaderGL::compileShader(unsigned int type, const ghc::filesystem::path &path) {
    // Load Source
    RageFile file;
    if (!file.Open(path))
        throw std::runtime_error(fmt::format("Error compiling shader {}: {}", path.string(), file.GetError()));
    std::string source;
    file.Read(source, file.GetFileSize());

    // Create and compile shader
    auto shaderID = glCreateShader(type);
    auto src = reinterpret_cast<const GLchar *>(source.c_str());
    glShaderSource(shaderID, 1, &src, nullptr);
    glCompileShader(shaderID);

    // Validate
    int success;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if(success == GL_TRUE)
        return shaderID;

    // Only reached if shader compilation fails.
    // Get length of error message
    GLint maxLength = 0;
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &maxLength);

    // Get error message and put in errorLog
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shaderID, maxLength, &maxLength, &errorLog[0]);

    glDeleteShader(shaderID); // Don't leak the shader.
    throw std::runtime_error(fmt::format("Shader compilation failed: {}", errorLog.data()));
}

bool ShaderGL::isActive() {
    return this->programID != 0;
}
