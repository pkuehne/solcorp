
#include "site_construction.h"
#include "construction_window.h"
// #include "modules/engine/render.h"
#include "modules/site/helpers.h"
#include "site.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <flecs.h>

enum LocationInfo {
  TileEmpty = 0,
  TileBuilding = 1,
  TileConstruction = 2,
};

///@brief Task launched when the location of Construction Sites needs updating
void systemUpdateConstructionSiteLocations(flecs::entity entity, Site &site) {
  spdlog::debug("Updating Construction Sites for {}", entity.name().c_str());

  auto world = entity.world();

  // Remove the old sites
  entity.children([](flecs::entity e) {
    if (e.has<ConstructionSite>()) {
      spdlog::debug("Removing old construction site {}", e.name().c_str());
      e.destruct();
    }
  });

  /// Location new sites
  flecs::entity currentSite;
  world.each<CurrentSite>(
      [&](flecs::entity e, CurrentSite) { currentSite = e; });

  std::vector<LocationInfo> locationMap(site.width * site.height,
                                        LocationInfo::TileEmpty);

  auto currentBuildings =
      world.query_builder<SiteLocation>().with<CurrentSite>().up().build();

  currentBuildings.each([&](flecs::entity e, SiteLocation &location) {
    if (e.has<Building>()) {
      spdlog::debug("Found building {} at {} {}", e.name().c_str(), location.x,
                    location.y);
      locationMap[location.y * site.height + location.x] =
          LocationInfo::TileBuilding;
    }
  });

  auto loc = [&site](size_t y, size_t x) -> size_t {
    return y * site.height + x;
  };

  auto is_valid = [&site](size_t y, size_t x) -> bool {
    if (y > site.height || x > site.width) {
      return false;
    }
    return true;
  };

  auto setConstruction = [&](size_t y, size_t x) {
    if (!is_valid(y, x)) {
      return;
    }
    if (locationMap[loc(y, x)] == LocationInfo::TileEmpty) {
      locationMap[loc(y, x)] = LocationInfo::TileConstruction;
    }
  };

  bool empty_site = true;
  for (size_t y = 0; y < site.height; y++) {
    for (size_t x = 0; x < site.width; x++) {
      if (locationMap[loc(y, x)] != LocationInfo::TileBuilding) {
        continue;
      }
      setConstruction(y - 1, x);
      setConstruction(y + 1, x);
      setConstruction(y, x - 1);
      setConstruction(y, x + 1);
      empty_site = false;
    }
  }

  if (empty_site) {
    spdlog::debug("Site is empty, adding construction site");
    setConstruction(site.height / 2, site.width / 2);
  }

  for (unsigned int y = 0; y < site.height; y++) {
    for (unsigned int x = 0; x < site.width; x++) {
      if (locationMap[loc(y, x)] != LocationInfo::TileConstruction) {
        continue;
      }

      // Create Construction Site
      spdlog::debug("Creating construction site at {} {}", x, y);
      instantiateConstructionSite(world, x, y, currentSite);
    }
  }

  entity.remove<ConstructionSiteNeedsUpdating>();
}
