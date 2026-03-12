#pragma once
#include <glad/glad.h>
#include <string>

// RAII wrapper for a compiled + linked OpenGL shader program.
// Non-copyable; owns the GL program handle.
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&)            = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    // Load, compile, and link from file paths.
    void load(const std::string& vertPath, const std::string& fragPath);

    void use() const;
    GLuint id() const { return m_id; }

    void setInt  (const char* name, int   v)   const;
    void setFloat(const char* name, float v)   const;
    void setVec2 (const char* name, float x, float y) const;

private:
    GLuint m_id = 0;

    static GLuint compileShader(GLenum type, const std::string& source);
    static std::string readFile(const std::string& path);
};
