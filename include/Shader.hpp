#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>

#if defined(LINUX) || defined(MINGW)
    #include <SDL2/SDL.h>
#else // This works for Mac
    #include <SDL.h>
#endif

#include <glad/glad.h>

class Shader{
public:
    // Shader constructor
    Shader();
    // Shader Destructor
    ~Shader();
    // Use this shader in our pipeline.
    void Bind() const;
    // Remove shader from our pipeline
    void Unbind() const;
    // Load a shader
    std::string LoadShader(const std::string& fname);
    // Create a Shader from a loaded vertex and fragment shader
    void CreateShader(const std::string& vertexShaderSource, const std::string& fragmentShaderSource);
    // return the shader id
    GLuint GetID() const;
    // Set our uniforms for our shader.
    void SetUniformMatrix4fv(const GLchar* name, const GLfloat* value);
	void SetUniform3f(const GLchar* name, float v0, float v1, float v2);
    void SetUniform1i(const GLchar* name, int value);
    void SetUniform1f(const GLchar* name, float value);

private:
    // Compiles loaded shaders
    unsigned int CompileShader(unsigned int type, const std::string& source);
    // Makes sure shaders 'linked' successfully
    bool CheckLinkStatus(GLuint programID);
    // Shader loading utility programs
    void PrintProgramLog( GLuint program );
    void PrintShaderLog( GLuint shader );
    // Logs an error message 
    void Log(const char* system, const char* message);
    // The unique shaderID
    GLuint m_shaderID;
};

#endif

