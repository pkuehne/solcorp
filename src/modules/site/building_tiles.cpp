#include "building_tiles.h"

#include <algorithm>
#include <array>

TileRole tileRoleAt(int x, int y, int w, int h) {
  constexpr std::array ROLES{
      std::array{TileRole::TopLeft, TileRole::Top, TileRole::TopRight},
      std::array{TileRole::Left, TileRole::Centre, TileRole::Right},
      std::array{TileRole::BottomLeft, TileRole::Bottom, TileRole::BottomRight},
  };
  auto band = [](int i, int size) {
    if (i == 0) {
      return 0;
    }
    return i == size - 1 ? 2 : 1;
  };
  return ROLES.at(band(y, h)).at(band(x, w));
}

std::vector<const BuildingTile *>
eligibleTiles(const BuildingTiles &tiles, TileRole role,
              const std::vector<std::string> &active_flags) {
  std::vector<const BuildingTile *> eligible;
  for (const BuildingTile &tile : tiles.tiles) {
    const bool fills_role =
        std::ranges::find(tile.roles, role) != tile.roles.end();
    const bool predicate_holds =
        tile.predicate.empty() ||
        std::ranges::find(active_flags, tile.predicate) != active_flags.end();
    if (fills_role && predicate_holds) {
      eligible.push_back(&tile);
    }
  }
  return eligible;
}

const BuildingTile *
pickTile(const std::vector<const BuildingTile *> &candidates, double roll) {
  double total = 0.0;
  for (const BuildingTile *tile : candidates) {
    total += tile->weight;
  }

  double threshold = roll * total;
  for (const BuildingTile *tile : candidates) {
    if (threshold < tile->weight) {
      return tile;
    }
    threshold -= tile->weight;
  }
  return candidates.empty() ? nullptr : candidates.back();
}
