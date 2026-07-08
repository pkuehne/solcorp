#include "modules/lua/rocket_data.h"

namespace {

RocketDef parseRocket(const std::string &id, const ModValue &rocket) {
  RocketDef def;
  def.id = id;
  def.name = rocket.getString("name").value_or(id);
  def.cost = static_cast<int>(rocket.getInt("cost").value_or(0));

  rocket.forEachArrayElement("orbits", [&](const ModValue &orbit) {
    if (orbit.isTable()) {
      def.target_orbits[orbit.getString("target").value_or("")] =
          static_cast<uint32_t>(orbit.getInt("mass").value_or(0));
    }
  });

  return def;
}

} // namespace

std::vector<RocketDef> parseRocketData(const ModValue &root) {
  std::vector<RocketDef> rockets;

  root.forEachEntry([&](const std::string &id, const ModValue &value) {
    if (value.isTable()) {
      rockets.push_back(parseRocket(id, value));
    }
  });

  return rockets;
}
