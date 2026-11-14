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
#include <string>
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
