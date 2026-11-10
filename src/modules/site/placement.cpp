#include "placement.h"
#include "connector.h"
#include "site.h"
#include <cstddef>
#include <utility>

bool OccupancyGrid::inBounds(int x, int y) const {
  return x >= 0 && y >= 0 && std::cmp_less(x, width) &&
         std::cmp_less(y, height);
}

CellContent OccupancyGrid::at(int x, int y) const {
  if (!inBounds(x, y)) {
    return CellContent::Empty;
  }
  return cells[(static_cast<size_t>(y) * width) + static_cast<size_t>(x)];
}

OccupancyGrid buildOccupancyGrid(flecs::entity site) {
  const auto &dimensions = site.get<Site>();
  OccupancyGrid grid{
      .width = dimensions.width,
      .height = dimensions.height,
      .cells = std::vector<CellContent>(static_cast<size_t>(dimensions.width) *
                                            dimensions.height,
                                        CellContent::Empty)};

  auto fill = [&grid](int x, int y, CellContent content) {
    if (grid.inBounds(x, y)) {
      grid.cells[(static_cast<size_t>(y) * grid.width) +
                 static_cast<size_t>(x)] = content;
    }
  };

  site.children([&](flecs::entity child) {
    const auto *location = child.try_get<SiteLocation>();
    if (location == nullptr) {
      return;
    }
    if (child.has<ConnectorTile>()) {
      fill(location->x, location->y, CellContent::Connector);
    } else if (child.has<Building>()) {
      const auto *footprint = child.try_get<Footprint>();
      const Footprint size = footprint != nullptr ? *footprint : Footprint{};
      for (int dy = 0; std::cmp_less(dy, size.h); ++dy) {
        for (int dx = 0; std::cmp_less(dx, size.w); ++dx) {
          fill(location->x + dx, location->y + dy, CellContent::Building);
        }
      }
    }
  });

  return grid;
}
