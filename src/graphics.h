#pragma once

#include "components.h"
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace flecs {
struct iter;
struct entity;
struct world;
} // namespace flecs

/// @brief Component to store SDL2 renderer information
struct Renderer {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
};

/// @brief Stores an SDL2 texture and its size information
struct Texture {
  SDL_Texture *ptr = nullptr;
  int width = 0;
  int height = 0;
  unsigned int rows = 0;
  unsigned int cols = 0;
};

struct Sprite {
  std::string texture;
  int tile = 0;
};

/// @brief Used to update Sprite component with the next frame in a texture
struct Animation {
  int fps = 0;           // Target FPS of the animation
  double elapsed = 0.0f; // How many milliseconds since frame was changed
};

void initialiseGraphics(flecs::world &world);
Texture loadTexture(const std::string &name, flecs::world &world);

// Systems
void systemRenderClear(const Renderer &r);
void systemRenderPresent(const Renderer &r);
void systemRenderSprite(flecs::entity e, const Sprite &sprite,
                        const Position &target, const Renderer &renderer);
// void systemRenderTileMap(flecs::entity e, const TileMap &map, const Renderer
// &renderer, const Constants &c); void systemUpdateAnimation(flecs::entity
// e, Animation &animation, Sprite &sprite);
