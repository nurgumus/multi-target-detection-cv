#include "ShaderProgram.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

ShaderProgram::~ShaderProgram() {
    if (m_id) glDeleteProgram(m_id);
}

std::string ShaderProgram::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("ShaderProgram: cannot open \"" + path + "\"");
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint ShaderProgram::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile error:\n") + log.data());
    }
    return shader;
}

void ShaderProgram::load(const std::string& vertPath, const std::string& fragPath) {
    GLuint vert = compileShader(GL_VERTEX_SHADER,   readFile(vertPath));
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, readFile(fragPath));

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);

    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(m_id, len, nullptr, log.data());
        glDeleteProgram(m_id);
        m_id = 0;
        throw std::runtime_error(std::string("Shader link error:\n") + log.data());
    }
}

void ShaderProgram::use() const {
    glUseProgram(m_id);
}

void ShaderProgram::setInt(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(m_id, name), v);
}

void ShaderProgram::setFloat(const char* name, float v) const {
    glUniform1f(glGetUniformLocation(m_id, name), v);
}

void ShaderProgram::setVec2(const char* name, float x, float y) const {
    glUniform2f(glGetUniformLocation(m_id, name), x, y);
}
