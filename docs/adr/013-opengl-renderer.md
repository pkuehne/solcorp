# 013 — OpenGL Renderer

**Status:** Accepted

## Context

The game drew sprites, text and tiles with SDL2's 2D `SDL_Renderer`, and ImGui used the matching
`imgui_impl_sdlrenderer2` backend. `SDL_Renderer` hides the graphics API behind a fixed-function
blit (`SDL_RenderCopyExF`), so there is no way to add custom shaders — for lighting, weather,
day/night tinting or post-processing — without replacing it.

## Decision

Render through an OpenGL 3.3 core context created by SDL2:

- `initialiseGraphics` creates the window with `SDL_WINDOW_OPENGL` and an OpenGL context, and the
  `Renderer` singleton holds the context and a small textured-quad pipeline (shader program, VAO,
  VBO).
- `Texture` holds an OpenGL texture name (`Texture::id`). Images and TTF text are still decoded
  by SDL_image/SDL_ttf into surfaces and uploaded as RGBA textures.
- Sprites, text and tiles are drawn by `drawQuad` in `render.cpp`, which reproduces the semantics
  of `SDL_RenderCopyExF` (source rect, destination rect, clockwise rotation about the centre,
  flipping) so existing callers keep their behaviour.
- ImGui keeps the SDL2 platform backend and swaps its renderer backend for `imgui_impl_opengl3`.
- OpenGL functions are resolved at runtime through `SDL_GL_GetProcAddress` by a tiny X-macro loader
  in `engine/gl.h`, exposed as `gl::<Name>`. No OpenGL library or third-party loader (GLEW, glad) is
  linked, so no new build or packaging dependency is introduced.

This change deliberately stops at parity with the SDL renderer; it adds no shader extension point
yet. `gl::createProgram` is the building block a later change can use to add custom shaders.

## Alternatives considered

- **SDL_gpu / SDL3 GPU API.** Would need a move to SDL3 first — a much larger change.
- **bgfx / sokol.** Portable across graphics APIs but a sizeable new dependency for a 2D game that
  only needs a handful of draw calls.
- **GLEW or glad as a loader.** Standard, but adds a dependency to Nix, vcpkg and CMake for the ~35
  functions the engine calls.

## Consequences

- Custom shaders are now possible: compile them with `gl::createProgram` and draw with them.
- The game requires an OpenGL 3.3 driver. The old software-renderer fallback is gone; without a
  suitable driver the game fails at startup with a logged error.
- Each quad is still drawn with its own draw call, as it effectively was before. Batching can be
  added behind `drawQuad` if it becomes a bottleneck.
- New OpenGL calls must be added to `SC_GL_FUNCTIONS` in `engine/gl.h` before use.
