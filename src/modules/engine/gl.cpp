#include "gl.h"
#include <SDL.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace gl {

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define SC_GL_DEFINE(name) decltype(&::gl##name) name = nullptr;
SC_GL_FUNCTIONS(SC_GL_DEFINE)
#undef SC_GL_DEFINE

bool load() {
  bool ok = true;
#define SC_GL_LOAD(name)                                                       \
  name = reinterpret_cast<decltype(name)>(SDL_GL_GetProcAddress("gl" #name));  \
  if ((name) == nullptr) {                                                     \
    spdlog::error("Missing OpenGL function gl" #name);                         \
    ok = false;                                                                \
  }
  SC_GL_FUNCTIONS(SC_GL_LOAD)
#undef SC_GL_LOAD
  return ok;
}

namespace {

GLuint compileShader(GLenum type, const std::string &source) {
  GLuint shader = CreateShader(type);
  const char *src = source.c_str();
  ShaderSource(shader, 1, &src, nullptr);
  CompileShader(shader);

  GLint status = GL_FALSE;
  GetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    GLint length = 0;
    GetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(static_cast<size_t>(length) + 1, '\0');
    GetShaderInfoLog(shader, length, nullptr, log.data());
    spdlog::error("Failed to compile shader: {}", log.data());
    DeleteShader(shader);
    return 0;
  }
  return shader;
}

} // namespace

uint32_t createProgram(const std::string &vertex_source,
                       const std::string &fragment_source) {
  GLuint vertex = compileShader(GL_VERTEX_SHADER, vertex_source);
  GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragment_source);
  if (vertex == 0 || fragment == 0) {
    DeleteShader(vertex);
    DeleteShader(fragment);
    return 0;
  }

  GLuint program = CreateProgram();
  AttachShader(program, vertex);
  AttachShader(program, fragment);
  LinkProgram(program);
  // Shaders are reference-counted by the program once attached
  DeleteShader(vertex);
  DeleteShader(fragment);

  GLint status = GL_FALSE;
  GetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    GLint length = 0;
    GetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> log(static_cast<size_t>(length) + 1, '\0');
    GetProgramInfoLog(program, length, nullptr, log.data());
    spdlog::error("Failed to link shader program: {}", log.data());
    DeleteProgram(program);
    return 0;
  }
  return program;
}

} // namespace gl
