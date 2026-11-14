/**
 * @file tile_data.h
 * @brief Typed building tile definitions parsed from a mod's tiles.lua.
 *
 * @details
 * A mod may ship a `tiles.lua` that `return`s a map keyed by building id. Each
 * entry names the tileset the building is drawn from and declares its tiles as
 * id-keyed `TileDef`s (ADR 012). parseTileData turns that table into plain
 * structs; roles and surfaces stay as written so validation can report unknown
 * names with provenance.
 */
#pragma once

#include "modules/lua/mod_value.h"
#include "modules/site/building_tiles.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// @brief One tile a building may be drawn with (ADR 012).
struct TileDef {
  std::string id;                 ///< Stable id within the building.
  long long index = 0;            ///< Tile index into the building's tileset.
  std::vector<std::string> roles; ///< 9-slice roles this tile may fill.
  std::string surface;            ///< What may mount here; empty means none.
  double weight = 1.0;   ///< Relative selection weight among candidates.
  std::string predicate; ///< Named flag gating eligibility; empty = always.
  std::vector<std::string> tags;
};

/// @brief The tiles declared for one building type.
struct BuildingTilesDef {
  std::string building; ///< Building id (the map key).
  std::string tileset;  ///< Tileset every tile index refers to.
  std::vector<TileDef> tiles;
};

/**
 * @brief Parse a merged tiles table into per-building tile definitions.
 *
 * Pure: reads the value tree only, performs no ECS work. Table-valued fields of
 * a building entry are its tiles; missing tile fields take their defaults.
 */
std::vector<BuildingTilesDef> parseTileData(const ModValue &root);

/// @brief The role named `name` in tiles.lua, or nullopt if unknown.
std::optional<TileRole> parseTileRole(const std::string &name);

/// @brief The name tiles.lua uses for `role`.
std::string_view tileRoleName(TileRole role);

/// @brief The surface named `name` in tiles.lua (empty is None), or nullopt
/// if unknown.
std::optional<TileSurface> parseTileSurface(const std::string &name);

/**
 * @brief Content check for a single tile definition.
 * @return nullopt if the tile is usable; otherwise a human-readable reason
 * (logged and the tile skipped by the caller).
 */
std::optional<std::string> validateTileDef(const TileDef &tile);

/**
 * @brief The roles no tile in `tiles` can always fill.
 *
 * A role is covered by a tile that declares it and has no predicate, so a
 * building can be drawn whatever its flags. Unknown role names are ignored.
 */
std::vector<TileRole> missingRoles(const std::vector<TileDef> &tiles);
