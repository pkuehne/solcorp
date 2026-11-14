#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// @brief A position within a building's 9-slice footprint (ADR 012)
enum class TileRole : uint8_t {
  TopLeft,
  Top,
  TopRight,
  Left,
  Centre,
  Right,
  BottomLeft,
  Bottom,
  BottomRight,
};

/// @brief What may be mounted on a tile (ADR 012)
enum class TileSurface : uint8_t { None, Roof, SouthWall };

/// @brief One tile a building type may be drawn with (ADR 012)
struct BuildingTile {
  std::string id;
  uint32_t index = 0; ///< Into the tileset BuildingTiles points at
  std::vector<TileRole> roles;
  TileSurface surface = TileSurface::None;
  double weight = 1.0;
  std::string predicate; ///< Named flag gating eligibility; empty = always
  std::vector<std::string> tags;
};

/// @brief A building prefab's flat tile registry, as a pair with data.
/// prefab --BuildingTiles{tiles}--> tileset entity (ADR 012)
struct BuildingTiles {
  std::vector<BuildingTile> tiles;
};
