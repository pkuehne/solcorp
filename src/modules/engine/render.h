#pragma once

#include <SDL_ttf.h>
#include <flecs.h>
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 0;
};

struct Point {
  float x = 0;
  float y = 0;
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
  int width = 0; // signed int, because that's what SDL uses for texture sizes
  int height = 0;
};

enum SpriteFlip : uint8_t {
  None = 0,
  Horizontal = 1,
  Vertical = 2,
};

struct Sprite {
  flecs::entity texture;
  uint32_t tile = 0;
  int x = 0;
  int y = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  float scale = 1.0f;
  double rotation = 0.0f;
  SpriteFlip flip = SpriteFlip::None;
};

struct Font {
  TTF_Font *ptr = nullptr;
  std::string name;
  uint32_t point_size = 12;
};

struct Text {
  std::string text;
  Color color = {.r = 0, .g = 0, .b = 0, .a = 0};
  double rotation = 0.0f;
  SpriteFlip flip = SpriteFlip::None;
};

Texture loadTexture(const std::string &, flecs::world &);
Texture loadTexture(const unsigned char *data, unsigned int len,
                    const flecs::world &world);

void initialiseGraphics(flecs::world &);

void registerRender(flecs::world &);
