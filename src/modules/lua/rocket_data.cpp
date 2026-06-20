#include "modules/lua/rocket_data.h"

namespace {

RocketDef parseRocket(const std::string &name, const LuaTableView &rocket) {
  RocketDef def;
  def.name = name;
  def.cost = static_cast<int>(rocket.getInt("cost").value_or(0));

  rocket.forEachArrayElement("orbits", [&](const LuaValue &value) {
    if (auto orbit = value.asTable()) {
      def.target_orbits[orbit->getString("target").value_or("")] =
          static_cast<uint32_t>(orbit->getInt("mass").value_or(0));
    }
  });

  return def;
}

} // namespace

std::vector<RocketDef> parseRocketData(const LuaTableView &root) {
  std::vector<RocketDef> rockets;

  root.forEachEntry([&](const std::string &name, const LuaValue &value) {
    if (auto rocket = value.asTable()) {
      rockets.push_back(parseRocket(name, *rocket));
    }
  });

  return rockets;
}
