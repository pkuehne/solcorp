#include "modules/lua/tile_data.h"

#include <algorithm>
#include <array>
#include <utility>

namespace {

constexpr std::array ROLE_NAMES{
    std::pair{TileRole::TopLeft, std::string_view{"TopLeft"}},
    std::pair{TileRole::Top, std::string_view{"Top"}},
    std::pair{TileRole::TopRight, std::string_view{"TopRight"}},
    std::pair{TileRole::Left, std::string_view{"Left"}},
    std::pair{TileRole::Centre, std::string_view{"Centre"}},
    std::pair{TileRole::Right, std::string_view{"Right"}},
    std::pair{TileRole::BottomLeft, std::string_view{"BottomLeft"}},
    std::pair{TileRole::Bottom, std::string_view{"Bottom"}},
    std::pair{TileRole::BottomRight, std::string_view{"BottomRight"}},
};

constexpr std::array SURFACE_NAMES{
    std::pair{TileSurface::None, std::string_view{""}},
    std::pair{TileSurface::Roof, std::string_view{"Roof"}},
    std::pair{TileSurface::SouthWall, std::string_view{"SouthWall"}},
};

template <typename Enum, std::size_t N>
std::optional<Enum>
lookupName(const std::array<std::pair<Enum, std::string_view>, N> &names,
           const std::string &name) {
  auto it = std::ranges::find(names, name,
                              &std::pair<Enum, std::string_view>::second);
  return it == names.end() ? std::nullopt : std::optional{it->first};
}

std::vector<std::string> parseStrings(const ModValue &table, const char *key) {
  std::vector<std::string> strings;
  table.forEachArrayElement(key, [&](const ModValue &value) {
    if (auto string = value.asString()) {
      strings.push_back(*string);
    }
  });
  return strings;
}

TileDef parseTile(const std::string &id, const ModValue &tile) {
  TileDef def;
  def.id = id;
  def.index = tile.getInt("index").value_or(0);
  def.roles = parseStrings(tile, "roles");
  def.surface = tile.getString("surface").value_or("");
  def.weight = tile.getNumber("weight").value_or(1.0);
  def.predicate = tile.getString("predicate").value_or("");
  def.tags = parseStrings(tile, "tags");
  return def;
}

} // namespace

std::vector<BuildingTilesDef> parseTileData(const ModValue &root) {
  std::vector<BuildingTilesDef> buildings;

  root.forEachEntry([&](const std::string &building, const ModValue &entry) {
    if (!entry.isTable()) {
      return;
    }
    BuildingTilesDef def;
    def.building = building;
    def.tileset = entry.getString("tileset").value_or("");
    entry.forEachEntry([&](const std::string &id, const ModValue &tile) {
      if (tile.isTable()) {
        def.tiles.push_back(parseTile(id, tile));
      }
    });
    buildings.push_back(def);
  });

  return buildings;
}

std::optional<TileRole> parseTileRole(const std::string &name) {
  return lookupName(ROLE_NAMES, name);
}

std::string_view tileRoleName(TileRole role) {
  return std::ranges::find(ROLE_NAMES, role,
                           &std::pair<TileRole, std::string_view>::first)
      ->second;
}

std::optional<TileSurface> parseTileSurface(const std::string &name) {
  return lookupName(SURFACE_NAMES, name);
}

std::optional<std::string> validateTileDef(const TileDef &tile) {
  if (tile.index < 0) {
    return "index must not be negative";
  }
  if (tile.roles.empty()) {
    return "it declares no roles";
  }
  for (const std::string &role : tile.roles) {
    if (!parseTileRole(role)) {
      return "unknown role '" + role + "'";
    }
  }
  if (!parseTileSurface(tile.surface)) {
    return "unknown surface '" + tile.surface + "'";
  }
  if (tile.weight <= 0.0) {
    return "weight must be positive";
  }
  return std::nullopt;
}

std::vector<TileRole> missingRoles(const std::vector<TileDef> &tiles) {
  auto always_fills = [&](std::string_view name) {
    return std::ranges::any_of(tiles, [&](const TileDef &tile) {
      return tile.predicate.empty() &&
             std::ranges::find(tile.roles, name) != tile.roles.end();
    });
  };

  std::vector<TileRole> missing;
  for (const auto &[role, name] : ROLE_NAMES) {
    if (!always_fills(name)) {
      missing.push_back(role);
    }
  }
  return missing;
}
