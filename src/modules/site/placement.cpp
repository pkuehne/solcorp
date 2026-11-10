#include "placement.h"
#include "connector.h"
#include "site.h"
#include <algorithm>
#include <array>
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

namespace {

constexpr std::array<std::pair<int, int>, 4> ORTHOGONAL_STEPS = {
    {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}};

bool hasNeighbour(const OccupancyGrid &grid, int x, int y,
                  CellContent content) {
  return std::ranges::any_of(ORTHOGONAL_STEPS, [&](auto step) {
    return grid.at(x + step.first, y + step.second) == content;
  });
}

PlacementResult canPlaceBuilding(const OccupancyGrid &grid, int origin_x,
                                 int origin_y, Footprint footprint) {
  bool next_to_road = false;
  for (int y = origin_y; y < origin_y + footprint.h; ++y) {
    for (int x = origin_x; x < origin_x + footprint.w; ++x) {
      if (!grid.inBounds(x, y)) {
        return PlacementResult::OutOfBounds;
      }
      if (grid.at(x, y) != CellContent::Empty) {
        return PlacementResult::Occupied;
      }
      next_to_road =
          next_to_road || hasNeighbour(grid, x, y, CellContent::Connector);
    }
  }
  return next_to_road ? PlacementResult::Ok
                      : PlacementResult::NotConnectedToRoad;
}

PlacementResult canPlaceConnector(const OccupancyGrid &grid, int x, int y) {
  if (!grid.inBounds(x, y)) {
    return PlacementResult::OutOfBounds;
  }
  if (grid.at(x, y) != CellContent::Empty) {
    return PlacementResult::Occupied;
  }
  const bool empty_site = std::ranges::all_of(
      grid.cells, [](CellContent c) { return c == CellContent::Empty; });
  if (empty_site || hasNeighbour(grid, x, y, CellContent::Connector) ||
      hasNeighbour(grid, x, y, CellContent::Building)) {
    return PlacementResult::Ok;
  }
  return PlacementResult::ConnectorNotConnected;
}

} // namespace

PlacementResult canPlace(const OccupancyGrid &grid, PlacementKind kind,
                         SiteLocation origin, Footprint footprint) {
  if (kind == PlacementKind::Connector) {
    return canPlaceConnector(grid, origin.x, origin.y);
  }
  return canPlaceBuilding(grid, origin.x, origin.y, footprint);
}

PlacementResult checkFootprintSize(Footprint footprint,
                                   const FootprintLimits &limits) {
  const auto min_w = std::max(limits.min.w, MIN_BUILDING_FOOTPRINT.w);
  const auto min_h = std::max(limits.min.h, MIN_BUILDING_FOOTPRINT.h);
  if (footprint.w < min_w || footprint.h < min_h) {
    return PlacementResult::TooSmall;
  }
  if (footprint.w > limits.max.w || footprint.h > limits.max.h) {
    return PlacementResult::TooLarge;
  }
  return PlacementResult::Ok;
}
