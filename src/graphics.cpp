#include "graphics.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>
#include <flecs.h>
#include <spdlog/spdlog.h>

/// @brief Initialises the Renderer and Window
/// @returns A Renderer Component to be added to the World
void initialiseGraphics(flecs::world &world) {
  auto m_width = 1000;
  auto m_height = 800;
  Uint32 flags = 0;
  auto m_tileWidth = 32;
  auto m_tileHeight = 32;

  spdlog::info("Initialising Graphics");

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
    throw std::runtime_error("Failed to initialize SDL");
  }

  Renderer r;
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

/// @brief Loads the given texture into a Texture component
/// @param name The filename (including directory) to load
/// @param world The flecs world to interact with
/// @returns A Texture Component to be added to an entity
Texture loadTexture(const std::string &filename, flecs::world &world) {
  Texture texture;
  const Renderer *r = world.get<Renderer>();

  // Load image at specified path
  SDL_Surface *surface = IMG_Load(filename.c_str());
  if (surface == nullptr) {
    spdlog::error("Unable to load image {} SDL_image Error: {}", filename,
                  IMG_GetError());
    throw std::runtime_error("Failed to load texture");
  }
  auto magenta = SDL_MapRGB(surface->format, 255, 0, 255);
  SDL_SetColorKey(surface, SDL_TRUE, magenta); // Make it transparent
  texture.width = surface->w;
  texture.height = surface->h;

  // Create texture from surface pixels
  texture.ptr = SDL_CreateTextureFromSurface(r->renderer, surface);
  if (texture.ptr == nullptr) {
    spdlog::error("Unable to create texture from surface. SDL Error: {}",
                  SDL_GetError());
    throw std::runtime_error("Failed to create texture from surface");
  }
  // SDL_SetTextureBlendMode(texture.ptr, SDL_BLENDMODE_BLEND);

  // Get rid of old loaded surface
  SDL_FreeSurface(surface);

  return texture;
}

/// @brief System that clears the screen before a frame update
/// @param it
/// @param renderer The Render Component Singleton
void systemRenderClear(flecs::iter &, const Renderer *r) {
  SDL_RenderClear(r->renderer);
}

/// @brief System to present the rendering instructions from previous systems
/// @param it
/// @param renderer The Render Component Singleton
void systemRenderPresent(flecs::iter &, const Renderer *r) {
  SDL_RenderPresent(r->renderer);
}

std::string format_as(SDL_Rect r) {
  return fmt::format("({},{}) {}x{}", r.x, r.y, r.w, r.h);
}

SDL_Rect clipTileFromTexture(const Texture *texture, int tile) {
  int tileSize = texture->width / texture->cols;
  int tileCol = tile % texture->cols;
  int tileRow = (tile - (tileCol)) / texture->cols;

  SDL_Rect source = {tileCol * tileSize, tileRow * tileSize, tileSize,
                     tileSize};
  return source;
}

/// @brief System to render a given TileSprite to a RenderTarget
/// @param iter The iterator being processed
/// @param sprite Sprite information to render
/// @param target Where on screen to render the sprite
/// @param renderer The renderer used to draw on the screen
void systemRenderSprite(flecs::iter &iter, const Sprite *sprite,
                        const Position *target, const Renderer *renderer) {
  const u_int tileSize = 32;
  auto texture = iter.world().lookup(sprite->texture.c_str()).get<Texture>();
  int target_x = static_cast<int>(target->x * tileSize);
  int target_y = static_cast<int>(target->y * tileSize);
  SDL_Rect source = clipTileFromTexture(texture, sprite->tile);
  SDL_Rect destination = {target_x, target_y, tileSize, tileSize};

  // SDL_SetTextureColorMod(spite.texture, fg.Red(), fg.Green(), fg.Blue());
  SDL_RenderCopy(renderer->renderer, texture->ptr, &source, &destination);
}

// /// @brief System to render a tilemap (i.e. background tiles)
// /// @param e The tilemap entity itself
// /// @param map The TileMap component holding the dimensions, texture, etc
// /// @param renderer The renderer used to draw on the screen
// void systemRenderTileMap(flecs::entity e, const TileMap &map, const Renderer
// &renderer, const Constants &c)
// {
//     auto texture = e.world().lookup(map.texture.c_str()).get<Texture>();
//     for (int y = 0; y < map.tiles.size(); y++)
//     {
//         const std::vector<Tile> &line = map.tiles[y];
//         for (int x = 0; x < line.size(); x++)
//         {
//             const Tile &tile = map.tiles[y][x];
//             SDL_Rect source = clipTileFromTexture(texture, tile.sprite);
//             SDL_Rect destination = {x * c.tileSize, y * c.tileSize,
//             c.tileSize, c.tileSize};
//             // spdlog::info("Rendering {} -> {}", format_as(source),
//             format_as(destination)); SDL_RenderCopy(renderer.renderer,
//             texture->ptr, &source, &destination);
//         }
//     }
// }
//
// void systemUpdateAnimation(flecs::entity e, Animation &animation, Sprite
// &sprite)
// {
//     double dt = e.world().delta_time();
//     auto texture = e.world().lookup(sprite.texture.c_str()).get<Texture>();
//
//     animation.elapsed += dt;
//     if (animation.elapsed >= (1.0f / animation.fps))
//     {
//         animation.elapsed = 0.0f;
//         sprite.tile += 1;
//     }
//     if (sprite.tile >= texture->cols)
//     {
//         sprite.tile = 0;
//     }
// }
