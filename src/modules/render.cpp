#include "render.h"
#include "modules/phase.h"
#include "spdlog/spdlog.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>
#include <cstdlib>
#include <flecs.h>

void initialiseGraphics(flecs::world &);
void systemLoadTextures(flecs::iter &);
void systemRenderClear(const Renderer &);
void systemRenderPresent(const Renderer &r);
void systemRenderSprite(flecs::entity, const Sprite &, const Position &,
                        const Renderer &);
SDL_Rect clipTileFromTexture(const Texture *, int);
Texture loadTexture(const std::string &, flecs::world &);

RenderModule::RenderModule(flecs::world &world) {

  world.import <phase>();

  initialiseGraphics(world);

  flecs::entity PreRenderPhase = world.lookup("phase.PreRender");
  flecs::entity RenderPhase = world.lookup("phase.Render");
  flecs::entity PostRenderPhase = world.lookup("phase.PostRender");

  // Register components
  world.component<Position>().member<u_int>("x").member<u_int>("y");
  world.component<Renderer>();
  world.component<Texture>();
  world.component<Sprite>().member<std::string>("texture").member<int>("tile");

  // Register systems
  world.system("Load Textures").kind(flecs::OnStart).run(systemLoadTextures);

  world.system<const Renderer>("Render Begin")
      .term_at(0)
      .singleton()
      .kind(PreRenderPhase)
      .each(systemRenderClear);

  world.system<const Sprite, const Position, const Renderer>("Render Sprites")
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
/// @param iter Access to flecs world
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

/// @brief Initialises the Renderer and Window
/// @param iter Access to flecs world
/// @returns A Renderer Component to be added to the World
void systemLoadTextures(flecs::iter &iter) {
  auto world = iter.world();

  Texture t = loadTexture("../../textures/solcorp_buildings.png", world);
  t.cols = 1;
  t.rows = 4;
  world.entity("building_texture").set<Texture>(t);
}

/// @brief System that clears the screen before a frame update
/// @param renderer The Render Component Singleton
void systemRenderClear(const Renderer &r) { SDL_RenderClear(r.renderer); }

/// @brief System to present the rendering instructions from previous systems
/// @param renderer The Render Component Singleton
void systemRenderPresent(const Renderer &r) { SDL_RenderPresent(r.renderer); }

/// @brief System to render a given TileSprite to a RenderTarget
/// @param e The entity to which the sprite belongs
/// @param sprite Sprite information to render
/// @param target Where on screen to render the sprite
/// @param renderer The renderer used to draw on the screen
void systemRenderSprite(flecs::entity e, const Sprite &sprite,
                        const Position &target, const Renderer &renderer) {
  const u_int tileSize = 32;
  auto texture = e.world().lookup(sprite.texture.c_str()).get<Texture>();
  int target_x = static_cast<int>(target.x * tileSize);
  int target_y = static_cast<int>(target.y * tileSize);
  SDL_Rect source = clipTileFromTexture(texture, sprite.tile);
  SDL_Rect destination = {target_x, target_y, tileSize, tileSize};

  // SDL_SetTextureColorMod(spite.texture, fg.Red(), fg.Green(), fg.Blue());
  SDL_RenderCopy(renderer.renderer, texture->ptr, &source, &destination);
}

/// @brief Extracts a n*n tile rect from a tilemap
/// @param texture A pointer to the texture to use
/// @param tile The tile (number wraps at row-end)
/// @returns An SDL_Rect with the co-ordinates to clip from
SDL_Rect clipTileFromTexture(const Texture *texture, int tile) {
  int tileSize = texture->width / texture->cols;
  int tileCol = tile % texture->cols;
  int tileRow = (tile - (tileCol)) / texture->cols;

  SDL_Rect source = {tileCol * tileSize, tileRow * tileSize, tileSize,
                     tileSize};
  return source;
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
