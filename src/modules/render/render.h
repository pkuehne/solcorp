#pragma once

#include <flecs.h>
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

struct Point {
  int x = 0;
  int y = 0;
};

struct Transform {
  Point relativePosition;
  Point worldPosition;
};

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
};

struct TileMap {
  flecs::entity texture;
  unsigned int cols = 0;
  unsigned int rows = 0;
};

struct Sprite {
  flecs::entity texture;
  int tile = 0;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

Texture loadTexture(const std::string &, flecs::world &);
Sprite spriteFromTileMap(flecs::entity tileMapE, int tile);

struct RenderModule {
public:
  RenderModule(flecs::world &);
};
