#include "modules/lua/building_data.h"

#include <cstdint>

namespace {

FacilityDef parseFacility(const LuaTableView &facility) {
  FacilityDef def;
  def.name = facility.getString("name").value_or("");
  def.type = facility.getString("type").value_or("");
  return def;
}

BuildingDef parseBuilding(const std::string &id, const LuaTableView &building) {
  BuildingDef def;
  def.id = id;
  def.name = building.getString("name").value_or(id);

  building.withTable("sprite", [&](const LuaTableView &sprite) {
    def.texture = sprite.getString("texture").value_or("");
    def.rect.x = static_cast<int>(sprite.getInt("x").value_or(0));
    def.rect.y = static_cast<int>(sprite.getInt("y").value_or(0));
    def.rect.width = static_cast<uint32_t>(sprite.getInt("width").value_or(0));
    def.rect.height =
        static_cast<uint32_t>(sprite.getInt("height").value_or(0));
  });

  building.forEachArrayElement("facilities", [&](const LuaValue &value) {
    if (auto facility = value.asTable()) {
      def.facilities.push_back(parseFacility(*facility));
    }
  });

  return def;
}

} // namespace

std::vector<BuildingDef> parseBuildingData(const LuaTableView &root) {
  std::vector<BuildingDef> buildings;

  root.forEachEntry([&](const std::string &id, const LuaValue &value) {
    if (auto building = value.asTable()) {
      buildings.push_back(parseBuilding(id, *building));
    }
  });

  return buildings;
}
