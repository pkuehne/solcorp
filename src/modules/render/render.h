#pragma once

#include <flecs.h>
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

struct Position {
  u_int x = 0;
  u_int y = 0;
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
  unsigned int rows = 0;
  unsigned int cols = 0;
};

struct Sprite {
  std::string texture;
  int tile = 0;
};

struct RenderModule {
public:
  RenderModule(flecs::world &);
};
