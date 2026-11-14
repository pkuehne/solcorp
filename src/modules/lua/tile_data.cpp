#include "modules/lua/tile_data.h"

namespace {

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
