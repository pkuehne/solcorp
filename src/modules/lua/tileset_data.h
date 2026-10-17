/**
 * @file tileset_data.h
 * @brief Typed tileset definitions parsed from a mod's tilesets.lua.
 *
 * @details
 * A mod may ship a `tilesets.lua` that `return`s an id-keyed map of tilesets
 * (`<name> -> { texture = ..., tile_size = ... }`), each cutting a named
 * texture into equal tiles (ADR 013). parseTilesetData turns that table into
 * plain structs; applying them to the ECS lives separately so the parse is pure
 * and unit-testable without a flecs world.
 */
#pragma once

#include "modules/lua/mod_value.h"
#include <string>
#include <vector>

/// @brief A named tileset a mod declares.
struct TilesetDef {
  std::string name;    ///< Registry name (Tilesets::<name>), the map key.
  std::string texture; ///< Texture it cuts up (Textures::<texture>).
  int tile_width = 0;
  int tile_height = 0;
  std::string defined_by; ///< Mod provenance for log messages. Filled at
                          ///< selection time, not by the parser.
};

/**
 * @brief Parse a merged tilesets table into typed definitions.
 *
 * Pure: reads the value tree only, performs no ECS work. `tile_size` sets both
 * dimensions; `tile_width` / `tile_height` override either. Missing fields
 * default to empty / 0 and are rejected at apply time.
 */
std::vector<TilesetDef> parseTilesetData(const ModValue &root);
