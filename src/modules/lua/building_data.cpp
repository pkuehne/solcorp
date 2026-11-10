#include "modules/lua/building_data.h"

#include <cstdint>
#include <utility>

namespace {

FacilityDef parseFacility(const ModValue &facility) {
  FacilityDef def;
  def.name = facility.getString("name").value_or("");
  def.type = facility.getString("type").value_or("");
  return def;
}

std::optional<SizeDef> parseSize(const ModValue &building, const char *key) {
  std::optional<SizeDef> size;
  building.withTable(key, [&](const ModValue &value) {
    size = SizeDef{.w = value.getInt("w").value_or(0),
                   .h = value.getInt("h").value_or(0)};
  });
  return size;
}

std::optional<std::string> validateSize(const std::optional<SizeDef> &size,
                                        const char *key) {
  constexpr long long MAX_SIZE = UINT8_MAX;
  auto in_range = [](long long v) { return v >= 1 && v <= MAX_SIZE; };
  if (size && (!in_range(size->w) || !in_range(size->h))) {
    return std::string(key) + " must be between 1 and 255 in each dimension";
  }
  return std::nullopt;
}

BuildingDef parseBuilding(const std::string &id, const ModValue &building) {
  BuildingDef def;
  def.id = id;
  def.name = building.getString("name").value_or(id);
  def.hidden = building.getBool("hidden").value_or(false);
  def.fixed_size = building.getBool("fixed_size").value_or(false);
  def.min_size = parseSize(building, "min_size");
  def.max_size = parseSize(building, "max_size");

  building.withTable("tile", [&](const ModValue &tile) {
    def.tileset = tile.getString("tileset").value_or("");
    def.tile_index = static_cast<uint32_t>(tile.getInt("index").value_or(0));
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
  if (def.fixed_size && (def.min_size || def.max_size)) {
    return "fixed_size cannot be combined with min_size or max_size";
  }
  for (auto [size, key] : {std::pair{&def.min_size, "min_size"},
                           std::pair{&def.max_size, "max_size"}}) {
    if (auto reason = validateSize(*size, key)) {
      return reason;
    }
  }
  if (def.min_size && def.max_size &&
      (def.min_size->w > def.max_size->w ||
       def.min_size->h > def.max_size->h)) {
    return "min_size is larger than max_size";
  }
  for (const FacilityDef &facility : def.facilities) {
    if (!isKnownFacilityType(facility.type)) {
      return "facility '" + facility.name + "' has unknown type '" +
             facility.type + "'";
    }
  }
  return std::nullopt;
}
