#include "render.h"
#include "SDL_render.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/lua/lua.h"
#include "spdlog/spdlog.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <cstddef>
#include <cstdlib>
#include <flecs.h>

void systemApplyParentTransform(Transform &t, const Transform *parent);
void systemRenderClear(const Renderer &);
void systemRenderPresent(const Renderer &r);
void systemRenderSprite(flecs::entity, const Sprite &, const Transform &,
                        const Renderer &);
void systemCreateTextureForText(flecs::entity e, Text &text,
                                const Renderer &renderer);
void systemRenderText(flecs::entity, const Text &, const Texture &,
                      const Transform &, const Renderer &);

void registerRender(flecs::world &world) {

  register_component_lua<Color>(world, "Color", [](lua_State *L, int mt) {
    lua_register_field<&Color::r>(L, mt, "r");
    lua_register_field<&Color::g>(L, mt, "g");
    lua_register_field<&Color::b>(L, mt, "b");
    lua_register_field<&Color::a>(L, mt, "a");
  });
  register_component_lua<Point>(world, "Point", [](lua_State *L, int mt) {
    lua_register_field<&Point::x>(L, mt, "x");
    lua_register_field<&Point::y>(L, mt, "y");
  });
  register_component_lua<Transform>(world, "Transform", [](lua_State *L, int mt) {
    lua_getfield(L, mt, "__getters");
    lua_pushcfunction(L, [](lua_State *Lx) -> int {
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      lua_push_component(Lx, &static_cast<Transform *>(ud->ptr)->relativePosition,
                         "solcorp.Point", false, nullptr);
      return 1;
    });
    lua_setfield(L, -2, "relativePosition");
    lua_pushcfunction(L, [](lua_State *Lx) -> int {
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      lua_push_component(Lx, &static_cast<Transform *>(ud->ptr)->worldPosition,
                         "solcorp.Point", false, nullptr);
      return 1;
    });
    lua_setfield(L, -2, "worldPosition");
    lua_pop(L, 1);
  });
  register_component_lua<Sprite>(world, "Sprite", [](lua_State *L, int mt) {
    lua_register_field<&Sprite::x>(L, mt, "x");
    lua_register_field<&Sprite::y>(L, mt, "y");
    lua_register_field<&Sprite::width>(L, mt, "width");
    lua_register_field<&Sprite::height>(L, mt, "height");
    lua_register_field<&Sprite::rotation>(L, mt, "rotation");
    lua_register_field<&Sprite::flip>(L, mt, "flip");
    lua_register_field<&Sprite::texture>(L, mt, "texture");
  });
  register_component_lua<Text>(world, "Text", [](lua_State *L, int mt) {
    lua_register_field<&Text::text>(L, mt, "text");
    lua_register_field<&Text::rotation>(L, mt, "rotation");
    lua_register_field<&Text::flip>(L, mt, "flip");
    lua_getfield(L, mt, "__getters");
    lua_pushcfunction(L, [](lua_State *Lx) -> int {
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      lua_push_component(Lx, &static_cast<Text *>(ud->ptr)->color,
                         "solcorp.Color", false, nullptr);
      return 1;
    });
    lua_setfield(L, -2, "color");
    lua_pop(L, 1);
  });

  auto scope = world.set_scope(0);
  world.entity("Textures");
  auto fonts = world.entity("Fonts");
  world.set_scope(scope);

  Font defaultFont;
  defaultFont.name = "fonts/RobotoMono-Medium.ttf";
  defaultFont.point_size = 18;
  defaultFont.ptr =
      TTF_OpenFont(defaultFont.name.c_str(), defaultFont.point_size);
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
      .kind(PostRenderPhase)
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

  Renderer r;
  flags |= SDL_WINDOW_ALLOW_HIGHDPI;
  r.window = SDL_CreateWindow("Sol, Corp", 0, 0, m_width, m_height, flags);
  if (r.window == nullptr) {
    spdlog::error("Failed to create SDL Window: {}", SDL_GetError());
    throw std::runtime_error("Failed to create SDL Window");
  }

  r.renderer = SDL_CreateRenderer(r.window, -1, SDL_RENDERER_ACCELERATED);
  if (r.renderer == nullptr) {
    spdlog::warn("Hardware renderer unavailable ({}), falling back to software",
                 SDL_GetError());
    r.renderer = SDL_CreateRenderer(r.window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (r.renderer == nullptr) {
    spdlog::error("Failed to create SDL Renderer: {}", SDL_GetError());
    throw std::runtime_error("Failed to create SDL Renderer");
  }
  SDL_SetRenderDrawBlendMode(r.renderer, SDL_BLENDMODE_BLEND);
  SDL_GetRendererOutputSize(r.renderer, &m_width, &m_height);
  SDL_SetRenderDrawColor(r.renderer, 42, 80, 35, SDL_ALPHA_OPAQUE);
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
void systemRenderClear(const Renderer &r) { SDL_RenderClear(r.renderer); }

/// @brief System to present the rendering instructions from previous systems
/// @param r The Render Component Singleton
void systemRenderPresent(const Renderer &r) { SDL_RenderPresent(r.renderer); }

/// @brief System to render a given TileSprite to a RenderTarget
/// @param e The entity to which the sprite belongs
/// @param sprite Sprite information to render
/// @param target Where on screen to render the sprite
/// @param renderer The renderer used to draw on the screen
void systemRenderSprite(flecs::entity, const Sprite &sprite,
                        const Transform &target, const Renderer &renderer) {
  SDL_Rect source = {sprite.x, sprite.y, sprite.width, sprite.height};
  SDL_FRect destination = {target.worldPosition.x, target.worldPosition.y,
                           sprite.width * sprite.scale,
                           sprite.height * sprite.scale};
  if (!sprite.texture.is_valid()) {
    spdlog::error("Sprite has no texture assigned");
    return;
  }
  auto t = sprite.texture.get<Texture>();
  if (t.ptr == nullptr) {
    spdlog::error("Texture pointer is null");
    return;
  }

  SDL_RenderCopyExF(renderer.renderer, t.ptr, &source, &destination,
                    sprite.rotation, NULL,
                    static_cast<SDL_RendererFlip>(sprite.flip));
}

void systemCreateTextureForText(flecs::entity e, Text &text,
                                const Renderer &renderer) {

  auto world = e.world();
  auto df = world.lookup("Fonts::Default");
  if (!df) {
    spdlog::error("Default font not found in world");
    return;
  }

  auto font = df.get<Font>();
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
  auto texture = SDL_CreateTextureFromSurface(renderer.renderer, surface);
  if (!texture) {
    spdlog::error("Failed to create text texture: {}", SDL_GetError());
    return;
  }
  e.set<Texture>({texture, surface->w, surface->h});
}

void systemRenderText(flecs::entity e, const Text &text, const Texture &texture,
                      const Transform &target, const Renderer &renderer) {
  auto world = e.world();

  SDL_Rect source = {0, 0, texture.width, texture.height};
  SDL_FRect destination = {target.worldPosition.x * 1.0f,
                           target.worldPosition.y * 1.0f, texture.width * 1.0f,
                           texture.height * 1.0f};
  SDL_RenderCopyExF(renderer.renderer, texture.ptr, &source, &destination,
                    text.rotation, NULL,
                    static_cast<SDL_RendererFlip>(text.flip));
}

/// @brief Loads the given texture into a Texture component
/// @param filename The filename (including directory) to load
/// @param world The flecs world to interact with
/// @returns A Texture Component to be added to an entity
Texture loadTexture(const std::string &filename, flecs::world &world) {
  Texture texture;
  const Renderer *r = world.try_get<Renderer>();
  if (r == nullptr) {
    spdlog::error("Renderer not set in world");
    return texture;
  }
  texture.ptr = IMG_LoadTexture(r->renderer, filename.c_str());
  if (texture.ptr == nullptr) {
    spdlog::error("Unable to create texture from surface. SDL Error: {}",
                  SDL_GetError());
    throw std::runtime_error("Failed to create texture from surface");
  }
  SDL_QueryTexture(texture.ptr, NULL, NULL, &texture.width, &texture.height);

  return texture;
}

Texture loadTexture(const unsigned char *data, unsigned int len,
                    const flecs::world &world) {
  Texture texture;
  const Renderer *r = world.try_get<Renderer>();
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

  texture.ptr = SDL_CreateTextureFromSurface(r->renderer, surface);
  SDL_FreeSurface(surface);

  return texture;
}
