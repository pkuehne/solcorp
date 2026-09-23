#pragma once

// Minimal OpenGL function loader.
//
// Every entry point is resolved at runtime through SDL_GL_GetProcAddress, so
// the game does not link against an OpenGL library and needs no third-party
// loader. The prototypes are only used to derive the function pointer types.
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#include <cstdint>
#include <string>

// X-macro list of every OpenGL function the engine uses. Add new entries here
// before calling them; each is exposed as gl::<Name> (e.g. gl::Clear).
#define SC_GL_FUNCTIONS(X)                                                     \
  X(Clear)                                                                     \
  X(ClearColor)                                                                \
  X(Viewport)                                                                  \
  X(Enable)                                                                    \
  X(BlendFunc)                                                                 \
  X(GenTextures)                                                               \
  X(DeleteTextures)                                                            \
  X(BindTexture)                                                               \
  X(TexImage2D)                                                                \
  X(TexParameteri)                                                             \
  X(PixelStorei)                                                               \
  X(DrawArrays)                                                                \
  X(CreateShader)                                                              \
  X(ShaderSource)                                                              \
  X(CompileShader)                                                             \
  X(GetShaderiv)                                                               \
  X(GetShaderInfoLog)                                                          \
  X(DeleteShader)                                                              \
  X(CreateProgram)                                                             \
  X(AttachShader)                                                              \
  X(LinkProgram)                                                               \
  X(GetProgramiv)                                                              \
  X(GetProgramInfoLog)                                                         \
  X(DeleteProgram)                                                             \
  X(UseProgram)                                                                \
  X(GetUniformLocation)                                                        \
  X(Uniform1i)                                                                 \
  X(Uniform2f)                                                                 \
  X(GenVertexArrays)                                                           \
  X(BindVertexArray)                                                           \
  X(GenBuffers)                                                                \
  X(BindBuffer)                                                                \
  X(BufferData)                                                                \
  X(BufferSubData)                                                             \
  X(VertexAttribPointer)                                                       \
  X(EnableVertexAttribArray)

namespace gl {

// The argument names a declaration, so it cannot be parenthesised
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define SC_GL_DECLARE(name) extern decltype(&::gl##name) name;
SC_GL_FUNCTIONS(SC_GL_DECLARE)
#undef SC_GL_DECLARE

/// @brief Resolves all OpenGL function pointers for the current context
/// @returns false (after logging the missing function) if any is unavailable
bool load();

/// @brief Compiles and links a shader program from GLSL sources
/// @param vertex_source Source of the vertex shader
/// @param fragment_source Source of the fragment shader
/// @returns The program handle, or 0 on failure (errors are logged)
uint32_t createProgram(const std::string &vertex_source,
                       const std::string &fragment_source);

} // namespace gl
