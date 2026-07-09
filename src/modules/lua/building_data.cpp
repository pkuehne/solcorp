#include "modules/lua/building_data.h"

#include <cstdint>

namespace {

FacilityDef parseFacility(const ModValue &facility) {
  FacilityDef def;
  def.name = facility.getString("name").value_or("");
  def.type = facility.getString("type").value_or("");
  return def;
}

BuildingDef parseBuilding(const std::string &id, const ModValue &building) {
  BuildingDef def;
  def.id = id;
  def.name = building.getString("name").value_or(id);
  def.hidden = building.getBool("hidden").value_or(false);

  building.withTable("sprite", [&](const ModValue &sprite) {
    def.texture = sprite.getString("texture").value_or("");
    def.rect.x = static_cast<int>(sprite.getInt("x").value_or(0));
    def.rect.y = static_cast<int>(sprite.getInt("y").value_or(0));
    def.rect.width = static_cast<uint32_t>(sprite.getInt("width").value_or(0));
    def.rect.height =
        static_cast<uint32_t>(sprite.getInt("height").value_or(0));
  });

  building.forEachArrayElement("facilities", [&](const ModValue &value) {
    if (value.isTable()) {
      def.facilities.push_back(parseFacility(value));
    }
  });

  return def;
}

} // namespace

std::vector<BuildingDef> parseBuildingData(const ModValue &root) {
  std::vector<BuildingDef> buildings;

  root.forEachEntry([&](const std::string &id, const ModValue &value) {
    if (value.isTable()) {
      buildings.push_back(parseBuilding(id, value));
    }
  });

  return buildings;
}

bool isKnownFacilityType(const std::string &type) {
  return type == "Launchpad" || type == "Office" || type == "Storage" ||
         type == "Manufacturing";
}

std::optional<std::string> validateBuildingDef(const BuildingDef &def) {
  for (const FacilityDef &facility : def.facilities) {
    if (!isKnownFacilityType(facility.type)) {
      return "facility '" + facility.name + "' has unknown type '" +
             facility.type + "'";
    }
  }
  return std::nullopt;
}
