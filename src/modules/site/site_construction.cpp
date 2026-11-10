
#include "site_construction.h"
#include "construction_window.h"
// #include "modules/engine/render.h"
#include "modules/site/helpers.h"
#include "placement.h"
#include "site.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <flecs.h>
#include <utility>
#include <vector>

///@brief Task launched when the location of Construction Sites needs updating
void systemUpdateConstructionSiteLocations(flecs::entity entity, Site &) {
  spdlog::debug("Updating Construction Sites for {}", entity.name().c_str());

  auto world = entity.world();

  // Remove the old sites
  entity.children([](flecs::entity e) {
    if (e.has<ConstructionSite>()) {
      spdlog::debug("Removing old construction site {}", e.name().c_str());
      e.destruct();
    }
  });

  const OccupancyGrid grid = buildOccupancyGrid(entity);
  std::vector<bool> construction(grid.cells.size(), false);

  auto setConstruction = [&](int x, int y) {
    if (grid.inBounds(x, y) && grid.at(x, y) == CellContent::Empty) {
      construction[(static_cast<size_t>(y) * grid.width) +
                   static_cast<size_t>(x)] = true;
    }
  };

  bool empty_site = true;
  for (int y = 0; std::cmp_less(y, grid.height); y++) {
    for (int x = 0; std::cmp_less(x, grid.width); x++) {
      if (grid.at(x, y) != CellContent::Building) {
        continue;
      }
      setConstruction(x, y - 1);
      setConstruction(x, y + 1);
      setConstruction(x - 1, y);
      setConstruction(x + 1, y);
      empty_site = false;
    }
  }

  if (empty_site) {
    spdlog::debug("Site is empty, adding construction site");
    setConstruction(grid.width / 2, grid.height / 2);
  }

  for (uint8_t y = 0; y < grid.height; y++) {
    for (uint8_t x = 0; x < grid.width; x++) {
      if (!construction[(static_cast<size_t>(y) * grid.width) + x]) {
        continue;
      }

      // Create Construction Site
      spdlog::debug("Creating construction site at {} {}", x, y);
      instantiateConstructionSite(world, SiteLocation{.x = x, .y = y}, entity);
    }
  }

  entity.remove<SiteNeedsRelayout>();
}
