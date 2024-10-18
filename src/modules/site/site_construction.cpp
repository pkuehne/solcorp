
#include "site_construction.h"
#include "construction_window.h"
#include "modules/render/render.h"
#include "site.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <flecs.h>

enum LocationInfo {
  Empty = 0,
  Building = 1,
  Construction = 2,
};

///@brief Task launched when the location of Construction Sites needs updating
void systemUpdateConstructionSiteLocations(flecs::entity entity, Site &site) {
  spdlog::debug("Updating Construction Sites for {}", entity.name().c_str());

  auto world = entity.world();

  // Remove the old sites
  entity.children([](flecs::entity e) {
    if (e.has<ConstructionSite>()) {
      e.destruct();
    }
  });

  /// Location new sites
  flecs::entity currentSite;
  world.each<CurrentSite>(
      [&](flecs::entity e, CurrentSite) { currentSite = e; });

  std::vector<LocationInfo> locationMap(site.width * site.height,
                                        LocationInfo::Empty);

  auto currentBuildings =
      world.query_builder<SiteLocation>().with<CurrentSite>().up().build();

  currentBuildings.each([&](flecs::entity, SiteLocation &location) {
    locationMap[location.y * site.height + location.x] = LocationInfo::Building;
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
    if (locationMap[loc(y, x)] == LocationInfo::Empty) {
      locationMap[loc(y, x)] = LocationInfo::Construction;
    }
  };

  for (size_t y = 0; y < site.height; y++) {
    for (size_t x = 0; x < site.width; x++) {
      if (locationMap[loc(y, x)] != LocationInfo::Building) {
        continue;
      }
      setConstruction(y - 1, x);
      setConstruction(y + 1, x);
      setConstruction(y, x + 1);
      setConstruction(y, x + 1);
    }
  }

  auto tm = world.lookup("BuildingTileMap");
  for (unsigned int y = 0; y < site.height; y++) {
    for (unsigned int x = 0; x < site.width; x++) {
      if (locationMap[loc(y, x)] != LocationInfo::Construction) {
        continue;
      }

      // Create Construction Site
      spdlog::debug("Creating construction site at {} {}", x, y);
      world
          .entity(fmt::format("Construction Site {}-{}/{}",
                              currentSite.name().c_str(), x, y)
                      .c_str())
          .set<SiteLocation>({x, y})
          .set<Sprite>(spriteFromTileMap(tm, 0))
          .add<ConstructionSite>()
          .child_of(currentSite);
    }
  }

  entity.remove<constructionSiteNeedsUpdating>();
}
