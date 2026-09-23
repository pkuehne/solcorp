#include "render.h"
#include "gl.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/lua/lua.h"
#include "spdlog/spdlog.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <flecs.h>
#include <numbers>
#include <utility>

void systemApplyParentTransform(Transform &t, const Transform *parent);
void systemRenderClear(const Renderer &);
void systemRenderPresent(const Renderer &);
void systemRenderSprite(flecs::entity, const Sprite &, const Transform &,
                        const Renderer &);
void systemCreateTextureForText(flecs::entity e, Text &text,
                                const Renderer &renderer);
void systemRenderText(flecs::entity, const Text &, const Texture &,
                      const Transform &, const Renderer &);

namespace {

constexpr const char *quad_vertex_shader = R"glsl(
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
uniform vec2 u_screen_size;
out vec2 v_uv;
void main() {
  // Map pixel co-ordinates (origin top-left, y down) to clip space
  vec2 ndc = a_position / u_screen_size * 2.0 - 1.0;
  gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
  v_uv = a_uv;
}
)glsl";

constexpr const char *quad_fragment_shader = R"glsl(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_texture;
out vec4 frag_color;
void main() {
  frag_color = texture(u_texture, v_uv);
}
)glsl";

/// @brief Creates the shader program and vertex buffers used by drawQuad
/// @param r The Renderer to store the GL objects in
void createQuadPipeline(Renderer &r) {
  r.quad_program = gl::createProgram(quad_vertex_shader, quad_fragment_shader);
  if (r.quad_program == 0) {
    throw std::runtime_error("Failed to create quad shader program");
  }
  r.screen_size_uniform =
      gl::GetUniformLocation(r.quad_program, "u_screen_size");
  gl::UseProgram(r.quad_program);
  gl::Uniform1i(gl::GetUniformLocation(r.quad_program, "u_texture"), 0);

  gl::GenVertexArrays(1, &r.quad_vao);
  gl::GenBuffers(1, &r.quad_vbo);
  gl::BindVertexArray(r.quad_vao);
  gl::BindBuffer(GL_ARRAY_BUFFER, r.quad_vbo);
  // Four vertices of (x, y, u, v), drawn as a triangle strip
  gl::BufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
  gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
  gl::EnableVertexAttribArray(0);
  // OpenGL takes the attribute's byte offset into the buffer as a pointer
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  const auto *uv_offset = reinterpret_cast<void *>(2 * sizeof(float));
  gl::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          uv_offset);
  gl::EnableVertexAttribArray(1);
  gl::BindVertexArray(0);
}

/// @brief Draws part of a texture to the screen, like SDL_RenderCopyExF
/// @param r The renderer used to draw on the screen
/// @param texture The texture to draw from
/// @param source The region of the texture to draw, in texels
/// @param dest Where on screen to draw, in pixels
/// @param rotation Clockwise rotation about the centre of dest, in degrees
/// @param flip Whether to mirror the texture horizontally and/or vertically
void drawQuad(const Renderer &r, const Texture &texture, const SDL_Rect &source,
              const SDL_FRect &dest, double rotation, SpriteFlip flip) {
  if (texture.width <= 0 || texture.height <= 0) {
    spdlog::error("Cannot draw texture {} without a size", texture.id);
    return;
  }
  const auto tex_w = static_cast<float>(texture.width);
  const auto tex_h = static_cast<float>(texture.height);
  float u0 = static_cast<float>(source.x) / tex_w;
  float u1 = static_cast<float>(source.x + source.w) / tex_w;
  float v0 = static_cast<float>(source.y) / tex_h;
  float v1 = static_cast<float>(source.y + source.h) / tex_h;
  if (flip & SpriteFlip::Horizontal) {
    std::swap(u0, u1);
  }
  if (flip & SpriteFlip::Vertical) {
    std::swap(v0, v1);
  }

  // With y pointing down, a positive angle turns clockwise on screen
  const auto radians = static_cast<float>(rotation * std::numbers::pi / 180.0);
  const float cos_r = std::cos(radians);
  const float sin_r = std::sin(radians);
  const float half_w = dest.w / 2.0f;
  const float half_h = dest.h / 2.0f;
  const float centre_x = dest.x + half_w;
  const float centre_y = dest.y + half_h;
  auto corner = [&](float dx, float dy) {
    return std::pair{centre_x + dx * cos_r - dy * sin_r,
                     centre_y + dx * sin_r + dy * cos_r};
  };
  const auto [x0, y0] = corner(-half_w, -half_h);
  const auto [x1, y1] = corner(half_w, -half_h);
  const auto [x2, y2] = corner(-half_w, half_h);
  const auto [x3, y3] = corner(half_w, half_h);
  const std::array<float, 16> vertices = {
      x0, y0, u0, v0, // top left
      x1, y1, u1, v0, // top right
      x2, y2, u0, v1, // bottom left
      x3, y3, u1, v1, // bottom right
  };

  gl::UseProgram(r.quad_program);
  gl::BindVertexArray(r.quad_vao);
  gl::BindBuffer(GL_ARRAY_BUFFER, r.quad_vbo);
  gl::BufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices.data());
  gl::BindTexture(GL_TEXTURE_2D, texture.id);
  gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/// @brief Uploads an SDL surface into a new OpenGL texture
/// @param surface The surface to upload; ownership stays with the caller
/// @returns The Texture, with id 0 if the upload failed
Texture createTextureFromSurface(SDL_Surface *surface) {
  SDL_Surface *rgba =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  if (rgba == nullptr) {
    spdlog::error("Failed to convert surface to RGBA: {}", SDL_GetError());
    return {};
  }

  Texture texture{.id = 0, .width = rgba->w, .height = rgba->h};
  gl::GenTextures(1, &texture.id);
  gl::BindTexture(GL_TEXTURE_2D, texture.id);
  // Nearest filtering matches the SDL renderer's default scale quality
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl::PixelStorei(GL_UNPACK_ROW_LENGTH, rgba->pitch / 4);
  gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgba->w, rgba->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba->pixels);
  gl::PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  SDL_FreeSurface(rgba);
  return texture;
}

} // namespace

void registerRender(flecs::world &world) {

  register_component_lua<Color>(world, "Color", [](LuaFieldBuilder<Color> &b) {
    b.field<&Color::r>("r")
        .field<&Color::g>("g")
        .field<&Color::b>("b")
        .field<&Color::a>("a");
  });
  register_component_lua<Point>(world, "Point", [](LuaFieldBuilder<Point> &b) {
    b.field<&Point::x>("x").field<&Point::y>("y");
  });
  register_component_lua<Transform>(
      world, "Transform", [](LuaFieldBuilder<Transform> &b) {
        b.nested<&Transform::relativePosition>({"relativePosition"}, {"Point"})
            .nested<&Transform::worldPosition>({"worldPosition"}, {"Point"});
      });
  register_component_lua<Sprite>(world, "Sprite",
                                 [](LuaFieldBuilder<Sprite> &b) {
                                   b.field<&Sprite::x>("x")
                                       .field<&Sprite::y>("y")
                                       .field<&Sprite::width>("width")
                                       .field<&Sprite::height>("height")
                                       .field<&Sprite::rotation>("rotation")
                                       .field<&Sprite::flip>("flip")
                                       .field<&Sprite::texture>("texture");
                                 });
  register_component_lua<Text>(world, "Text", [](LuaFieldBuilder<Text> &b) {
    b.field<&Text::text>("text")
        .field<&Text::rotation>("rotation")
        .field<&Text::flip>("flip")
        .nested<&Text::color>({"color"}, {"Color"});
  });

  auto scope = world.set_scope(0);
  auto textures = world.entity("Textures");
  auto fonts = world.entity("Fonts");
  world.set_scope(scope);

  static const std::filesystem::path asset_texture_dir("assets/textures");
  if (std::filesystem::is_directory(asset_texture_dir)) {
    for (const auto &entry :
         std::filesystem::directory_iterator(asset_texture_dir)) {
      if (entry.path().extension() == ".png") {
        auto name = entry.path().stem().string();
        world.entity(name.c_str())
            .child_of(textures)
            .set<Texture>(loadTexture(entry.path().string(), world));
        spdlog::debug("Loaded asset texture: {}", name);
      }
    }
  } else {
    spdlog::warn("Asset texture directory '{}' not found",
                 asset_texture_dir.string());
  }

  const auto &config = world.get<Config>();

  Font defaultFont;
  defaultFont.name = (std::filesystem::path("fonts") / config.font).string();
  defaultFont.point_size = config.font_size;
  defaultFont.ptr = TTF_OpenFont(defaultFont.name.c_str(),
                                 static_cast<int>(defaultFont.point_size));
  SC_ASSERT(defaultFont.ptr != nullptr, "Failed to load font '" +
                                            defaultFont.name +
                                            "': " + TTF_GetError());
  world.entity("Default").set<Font>(defaultFont).child_of(fonts);

  // Register systems
  world.system<Transform, const Transform *>("Apply Parent Transform")
      .term_at(1)
      .parent()
      .cascade()
      .kind(PreFramePhase)
      .each(systemApplyParentTransform);

  world.system<Text, const Renderer>("Create Texture for Text")
      .without<Texture>()
      .kind(PreFramePhase)
      .each(systemCreateTextureForText);

  world.system<const Renderer>("Render Begin")
      .kind(PreRenderPhase)
      .each(systemRenderClear);

  world.system<const Sprite, const Transform, const Renderer>("Render Sprites")
      .kind(RenderPhase)
      .each(systemRenderSprite);

  world
      .system<const Text, const Texture, const Transform, const Renderer>(
          "Render Text")
      .kind(RenderPhase)
      .each(systemRenderText);

  world.system<const Renderer>("Render End")
      .kind(PostFramePhase)
      .each(systemRenderPresent);
}

/// @brief Initialises the Renderer and Window
/// @param world Access to flecs world
/// @returns A Renderer Component to be added to the World
void initialiseGraphics(flecs::world &world) {

  auto m_width = 1000;
  auto m_height = 800;
  auto m_tileWidth = 32;
  auto m_tileHeight = 32;
  Uint32 flags = 0;

  spdlog::info("Initialising Graphics");

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
    throw std::runtime_error("Failed to initialize SDL");
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                      SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  Renderer r;
  flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
  r.window = SDL_CreateWindow("Sol, Corp", 0, 0, m_width, m_height, flags);
  if (r.window == nullptr) {
    spdlog::error("Failed to create SDL Window: {}", SDL_GetError());
    throw std::runtime_error("Failed to create SDL Window");
  }

  r.context = SDL_GL_CreateContext(r.window);
  if (r.context == nullptr) {
    spdlog::error("Failed to create OpenGL context: {}", SDL_GetError());
    throw std::runtime_error("Failed to create OpenGL context");
  }
  // The SDL renderer did not wait for vsync; flecs paces the frame rate
  SDL_GL_SetSwapInterval(0);
  if (!gl::load()) {
    throw std::runtime_error("Failed to load OpenGL functions");
  }
  createQuadPipeline(r);

  gl::Enable(GL_BLEND);
  gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl::ClearColor(42.0f / 255.0f, 80.0f / 255.0f, 35.0f / 255.0f, 1.0f);
  SDL_GL_GetDrawableSize(r.window, &m_width, &m_height);
  spdlog::info("Window ({}x{}) Tile ({}x{})", m_width, m_height, m_tileWidth,
               m_tileHeight);

  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    spdlog::error("Failed to initialize IMG: {}", IMG_GetError());
    throw std::runtime_error("Failed to initialize IMG");
  }

  if (TTF_Init()) {
    spdlog::error("Failed to initialize TTF: {}", TTF_GetError());
    throw std::runtime_error("Failed to initialize TTF");
  }

  world.set<Renderer>(r);
}

/// @brief Applies the parent's transform to the relative co-ordinates
/// @param[in/out] t The Transform to calculate worldPosition for
/// @param[in] parent The parent transform to use
void systemApplyParentTransform(Transform &t, const Transform *parent) {
  if (parent) {
    t.worldPosition.x = parent->worldPosition.x + t.relativePosition.x;
    t.worldPosition.y = parent->worldPosition.y + t.relativePosition.y;
  } else {
    t.worldPosition = t.relativePosition;
    t.worldPosition = t.relativePosition;
  }
}

/// @brief System that clears the screen before a frame update
/// @param r The Render Component Singleton
void systemRenderClear(const Renderer &r) {
  // Track the drawable size every frame so window resizes are picked up
  int width = 0;
  int height = 0;
  SDL_GL_GetDrawableSize(r.window, &width, &height);
  gl::Viewport(0, 0, width, height);
  gl::Clear(GL_COLOR_BUFFER_BIT);
  gl::UseProgram(r.quad_program);
  gl::Uniform2f(r.screen_size_uniform, static_cast<float>(width),
                static_cast<float>(height));
}

/// @brief System to present the rendering instructions from previous systems
/// @param r The Render Component Singleton
void systemRenderPresent(const Renderer &r) {
  SDL_GL_SwapWindow(r.window);
  SDL_ShowWindow(r.window);
}

/// @brief System to render a given TileSprite to a RenderTarget
/// @param e The entity to which the sprite belongs
/// @param sprite Sprite information to render
/// @param target Where on screen to render the sprite
/// @param renderer The renderer used to draw on the screen
void systemRenderSprite(flecs::entity, const Sprite &sprite,
                        const Transform &target, const Renderer &renderer) {
  SDL_Rect source = {sprite.x, sprite.y, static_cast<int>(sprite.width),
                     static_cast<int>(sprite.height)};
  SDL_FRect destination = {target.worldPosition.x, target.worldPosition.y,
                           static_cast<float>(sprite.width) * sprite.scale,
                           static_cast<float>(sprite.height) * sprite.scale};
  if (!sprite.texture.is_valid()) {
    spdlog::error("Sprite has no texture assigned");
    return;
  }
  auto t = sprite.texture.get<Texture>();
  if (t.id == 0) {
    spdlog::error("Texture pointer is null");
    return;
  }

  drawQuad(renderer, t, source, destination, sprite.rotation, sprite.flip);
}

void systemCreateTextureForText(flecs::entity e, Text &text, const Renderer &) {

  auto world = e.world();
  auto df = world.lookup("Fonts::Default");
  if (!df) {
    spdlog::error("Default font not found in world");
    return;
  }

  const auto &font = df.get<Font>();
  if (!font.ptr) {
    spdlog::warn("Cannot render text: default font not loaded");
    e.remove<Text>();
    return;
  }
  SDL_Color color = {text.color.r, text.color.g, text.color.b, text.color.a};

  SDL_Surface *surface =
      TTF_RenderText_Blended(font.ptr, text.text.c_str(), color);
  if (!surface) {
    spdlog::error("Failed to render text surface: {}", TTF_GetError());
    return;
  }
  auto texture = createTextureFromSurface(surface);
  SDL_FreeSurface(surface);
  if (texture.id == 0) {
    spdlog::error("Failed to create text texture");
    return;
  }
  e.set<Texture>(texture);
}

void systemRenderText(flecs::entity e, const Text &text, const Texture &texture,
                      const Transform &target, const Renderer &renderer) {
  auto world = e.world();

  SDL_Rect source = {0, 0, texture.width, texture.height};
  SDL_FRect destination = {target.worldPosition.x * 1.0f,
                           target.worldPosition.y * 1.0f,
                           static_cast<float>(texture.width) * 1.0f,
                           static_cast<float>(texture.height) * 1.0f};
  drawQuad(renderer, texture, source, destination, text.rotation, text.flip);
}

void renderTile(const Renderer &renderer, const Texture &texture, int src_col,
                int src_row, int tile_size, const Transform &transform,
                double rotation) {
  SDL_Rect source = {src_col * tile_size, src_row * tile_size, tile_size,
                     tile_size};
  SDL_FRect dest = {transform.worldPosition.x, transform.worldPosition.y,
                    static_cast<float>(tile_size),
                    static_cast<float>(tile_size)};
  drawQuad(renderer, texture, source, dest, rotation, SpriteFlip::None);
}

/// @brief Loads the given texture into a Texture component
/// @param filename The filename (including directory) to load
/// @param world The flecs world to interact with
/// @returns A Texture Component to be added to an entity
Texture loadTexture(const std::string &filename, flecs::world &world) {
  Texture texture;
  const auto *r = world.try_get<Renderer>();
  if (r == nullptr) {
    spdlog::error("Renderer not set in world");
    return texture;
  }
  SDL_Surface *surface = IMG_Load(filename.c_str());
  if (surface != nullptr) {
    texture = createTextureFromSurface(surface);
    SDL_FreeSurface(surface);
  }
  if (texture.id == 0) {
    spdlog::error("Unable to create texture from surface. SDL Error: {}",
                  SDL_GetError());
    throw std::runtime_error("Failed to create texture from surface");
  }

  return texture;
}

Texture loadTexture(const unsigned char *data, unsigned int len,
                    const flecs::world &world) {
  Texture texture;
  const auto *r = world.try_get<Renderer>();
  if (r == nullptr) {
    spdlog::error("Renderer not set in world");
    return texture;
  }
  SDL_RWops *rw = SDL_RWFromConstMem(data, static_cast<int>(len));
  if (!rw) {
    spdlog::error("SDL_RWFromConstMem failed: {}", SDL_GetError());
    return texture;
  }

  SDL_Surface *surface = IMG_Load_RW(rw, 1); // 1 = free rw automatically
  if (!surface) {
    spdlog::error("IMG_Load_RW failed: {}", IMG_GetError());
    return texture;
  }

  texture = createTextureFromSurface(surface);
  SDL_FreeSurface(surface);

  return texture;
}
