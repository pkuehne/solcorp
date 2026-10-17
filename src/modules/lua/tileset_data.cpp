#include "modules/lua/tileset_data.h"

std::vector<TilesetDef> parseTilesetData(const ModValue &root) {
  std::vector<TilesetDef> tilesets;

  root.forEachEntry([&](const std::string &name, const ModValue &value) {
    if (value.isTable()) {
      const auto tile_size = value.getInt("tile_size").value_or(0);
      TilesetDef def;
      def.name = name;
      def.texture = value.getString("texture").value_or("");
      def.tile_width =
          static_cast<int>(value.getInt("tile_width").value_or(tile_size));
      def.tile_height =
          static_cast<int>(value.getInt("tile_height").value_or(tile_size));
      tilesets.push_back(def);
    }
  });

  return tilesets;
}
