#pragma once

#include <SDL_ttf.h>
#include <flecs.h>
#include <spdlog/spdlog.h>
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

/// @brief Draws a single tile from a texture atlas at a world transform.
/// @param renderer The renderer used to draw on the screen
/// @param texture The source texture atlas
/// @param src_col Column of the tile within the atlas
/// @param src_row Row of the tile within the atlas
/// @param tile_size Edge length of one (square) tile, in pixels
/// @param transform World transform whose worldPosition is the draw destination
/// @param rotation Clockwise rotation applied to the tile, in degrees
void renderTile(const Renderer &renderer, const Texture &texture, int src_col,
                int src_row, int tile_size, const Transform &transform,
                double rotation);

/// @brief Draws one layer of a tiled entity by following a tileset relationship
/// to the texture that layer lives on.
///
/// Resolves the @p TilesetRel relationship target on @p e. If that target is
/// valid and holds a non-null Texture, the tile at (@p src_col, @p src_row) in
/// its atlas is drawn at @p transform. A missing relationship or a null texture
/// is a silent no-op, so a tile's layers (e.g. base, markings, embellishments)
/// can each be present or absent independently. Templated on the relationship
/// tag so any module can define its own layers (connector tilesets, multi-tile
/// building tilesets, ...) without the engine knowing about them.
///
/// The Renderer is fetched from @p e's world (it is a singleton), so callers
/// need not query it themselves.
///
/// @tparam TilesetRel Relationship tag whose target entity carries the layer's
///                    Texture (e.g. TilesetBase, TilesetMarkings)
/// @param e The entity being drawn; supplies the layer's texture (via its
///          @p TilesetRel target) and the world the Renderer is read from
/// @param src_col Column of the tile within the tileset atlas
/// @param src_row Row of the tile within the tileset atlas
/// @param tile_size Edge length of one (square) tile, in pixels
/// @param transform World transform whose worldPosition is the draw destination
/// @param rotation Clockwise rotation applied to the tile, in degrees
template <typename TilesetRel>
void renderTileLayer(flecs::entity e, int src_col, int src_row, int tile_size,
                     const Transform &transform, double rotation) {
  auto tileset = e.target<TilesetRel>();
  if (!tileset.is_valid()) {
    return;
  }
  const auto &texture = tileset.template get<Texture>();
  if (texture.ptr == nullptr) {
    spdlog::error("Tileset relationship target has null texture");
    return;
  }
  const auto *renderer = e.world().try_get<Renderer>();
  if (renderer == nullptr) {
    return;
  }
  renderTile(*renderer, texture, src_col, src_row, tile_size, transform,
             rotation);
}

void initialiseGraphics(flecs::world &);

void registerRender(flecs::world &);
