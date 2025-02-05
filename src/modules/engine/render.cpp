#include "render.h"
#include "SDL_render.h"
#include "modules/engine/engine.h"
#include "spdlog/spdlog.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>
#include <cstddef>
#include <cstdlib>
#include <flecs.h>

void systemApplyParentTransform(Transform &t, const Transform *parent);
void systemRenderClear(const Renderer &);
void systemRenderPresent(const Renderer &r);
void systemRenderSprite(flecs::entity, const Sprite &, const Transform &,
                        const Renderer &);
void registerRender(flecs::world &world) {

  // Register components
  world.component<Point>().member<int>("x").member<int>("y");
  world.component<Transform>()
      .member<Point>("relativePosition")
      .member<Point>("worldPosition");
  world.component<Renderer>();
  world.component<Texture>()
      .member<size_t>("ptr")
      .member<int>("width")
      .member<int>("height");
  world.component<TileMap>()
      .member<flecs::entity>("texture")
      .member<unsigned int>("cols")
      .member<unsigned int>("rows")
      .member<unsigned int>("tileSize");
  world.component<Sprite>()
      .member<flecs::entity>("texture")
      .member<int>("tile")
      .member<int>("x")
      .member<int>("y")
      .member<int>("width")
      .member<int>("height")
      .member<double>("rotation")
      .member<int>("flip");

  // Register systems
  world.system<Transform, const Transform *>("Apply Parent Transform")
      .term_at(1)
      .parent()
      .cascade()
      .kind(PreFramePhase)
      .each(systemApplyParentTransform);

  world.system<const Renderer>("Render Begin")
      .term_at(0)
      .singleton()
      .kind(PreRenderPhase)
      .each(systemRenderClear);

  world.system<const Sprite, const Transform, const Renderer>("Render Sprites")
      .term_at(2)
      .singleton()
      .kind(RenderPhase)
      .each(systemRenderSprite);

  world.system<const Renderer>("Render End")
      .term_at(0)
      .singleton()
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
    spdlog::error("Failed to create SDL Renderer: {}", SDL_GetError());
    throw std::runtime_error("Failed to create SDL Renderer");
  }
  SDL_SetRenderDrawBlendMode(r.renderer, SDL_BLENDMODE_BLEND);
  SDL_GetRendererOutputSize(r.renderer, &m_width, &m_height);
  SDL_SetRenderDrawColor(r.renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
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
  SDL_FRect destination = {
      target.worldPosition.x * 1.0f, target.worldPosition.y * 1.0f,
      sprite.width * sprite.scale, sprite.height * sprite.scale};
  auto t = sprite.texture.get<Texture>();

  SDL_RenderCopyExF(renderer.renderer, t->ptr, &source, &destination,
                    sprite.rotation, NULL,
                    static_cast<SDL_RendererFlip>(sprite.flip));
}

/// @brief Extracts a tile from a tilemap and generates a Sprite component
/// @param[in] tileMapE The tile map entity (has a @link TileMap component)
/// @param[in] tile The tile number (number wraps at row-end)
/// @returns A Sprite with the co-ordinates to clip tile from in the texture
Sprite spriteFromTileMap(flecs::entity tileMapE, int tile) {
  auto tileMap = tileMapE.get<TileMap>();

  int tileCol = tile % tileMap->cols;
  int tileRow = (tile - (tileCol)) / tileMap->cols;

  Sprite sprite;
  sprite.texture = tileMap->texture;
  sprite.x = tileCol * tileMap->tileSize;
  sprite.y = tileRow * tileMap->tileSize;
  sprite.width = tileMap->tileSize;
  sprite.height = tileMap->tileSize;

  return sprite;
}

/// @brief Loads the given texture into a Texture component
/// @param filename The filename (including directory) to load
/// @param world The flecs world to interact with
/// @returns A Texture Component to be added to an entity
Texture loadTexture(const std::string &filename, flecs::world &world) {
  Texture texture;
  const Renderer *r = world.get<Renderer>();
  texture.ptr = IMG_LoadTexture(r->renderer, filename.c_str());
  if (texture.ptr == nullptr) {
    spdlog::error("Unable to create texture from surface. SDL Error: {}",
                  SDL_GetError());
    throw std::runtime_error("Failed to create texture from surface");
  }
  SDL_QueryTexture(texture.ptr, NULL, NULL, &texture.width, &texture.height);

  return texture;
}

/// @brief Takes a texture and creates a TileMap component based on a square
/// tile size
/// @param[in] textureE The entity with a Texture component
/// @param[in] tileSize The size of the tiles in the map in pixels
/// @return A TileMap component
TileMap tileMapFromTexture(const flecs::entity &textureE, uint tileSize) {
  TileMap map;

  auto texture = textureE.get<Texture>();

  map.tileSize = tileSize;
  map.texture = textureE;
  map.cols = texture->width / tileSize;
  map.rows = texture->height / tileSize;

  return map;
}
