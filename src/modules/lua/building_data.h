/**
 * @file building_data.h
 * @brief Typed building/texture definitions parsed from a mod's buildings.lua.
 *
 * @details
 * A mod may ship a `buildings.lua` that `return`s an id-keyed map of building
 * prefab definitions (per ADR 011/012). parseBuildingData turns that table into
 * plain structs; applying the structs to the ECS lives separately (see
 * building_data.cpp) so the parse is pure and unit-testable without a flecs
 * world. Textures live in a separate textures.lua (see texture_data.h), loaded
 * first so building sprites can be validated against them.
 */
#pragma once

#include "modules/lua/lua_data.h"
#include <cstdint>
#include <string>
#include <vector>

/// @brief A sprite clip rectangle into a texture atlas (pixels).
struct SpriteClipRect {
  int x;
  int y;
  uint32_t width;
  uint32_t height;
};

/// @brief A facility within a building definition (name + facility type tag).
struct FacilityDef {
  std::string name;
  /// One of "Launchpad" | "Office" | "Storage" | "Manufacturing".
  std::string type;
};

/// @brief A building prefab definition parsed from a mod's buildings.lua.
struct BuildingDef {
  std::string id;        ///< Stable registry id (the map key).
  std::string name;      ///< Display name / prefab entity name.
  std::string texture;   ///< Named texture the sprite clips from.
  SpriteClipRect rect{}; ///< Sprite clip rectangle into that texture.
  std::vector<FacilityDef> facilities;
};

/**
 * @brief Parse a buildings.lua root table (an id-keyed map) into definitions.
 *
 * Pure: reads the table only, performs no ECS work. Missing fields default and
 * malformed entries are skipped leniently; content validation and logging
 * happen at apply time.
 */
std::vector<BuildingDef> parseBuildingData(const LuaTableView &root);
