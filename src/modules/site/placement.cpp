#include "placement.h"
#include "connector.h"
#include "site.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
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

std::string placementMessage(PlacementResult result) {
  switch (result) {
  case PlacementResult::Ok:
    return "";
  case PlacementResult::OutOfBounds:
    return "It doesn't fit on the site";
  case PlacementResult::Occupied:
    return "Something is already built there";
  case PlacementResult::TooSmall:
    return "It is too small";
  case PlacementResult::TooLarge:
    return "It is too large";
  case PlacementResult::NotConnectedToRoad:
    return "Buildings must be next to a road";
  case PlacementResult::ConnectorNotConnected:
    return "Roads must join a road or building";
  }
  return "";
}

DragShape dragShape(flecs::entity prefab) {
  if (prefab.has<ConnectorTile>()) {
    return DragShape::Run;
  }
  return prefab.has<FixedSize>() ? DragShape::None : DragShape::Rectangle;
}

FootprintLimits placementLimits(flecs::entity prefab) {
  const Footprint min = placementFootprint(prefab);
  if (dragShape(prefab) != DragShape::Rectangle) {
    return {.min = min, .max = min};
  }
  const auto *limits = prefab.try_get<FootprintLimits>();
  return {.min = min,
          .max = limits != nullptr ? limits->max : FootprintLimits{}.max};
}

Footprint placementFootprint(flecs::entity prefab) {
  if (dragShape(prefab) != DragShape::Rectangle) {
    return {};
  }
  const auto *limits = prefab.try_get<FootprintLimits>();
  const Footprint min =
      limits != nullptr ? limits->min : MIN_BUILDING_FOOTPRINT;
  return {.w = std::max(min.w, MIN_BUILDING_FOOTPRINT.w),
          .h = std::max(min.h, MIN_BUILDING_FOOTPRINT.h)};
}

namespace {

struct Span {
  int start;
  int length;
};

struct AxisDrag {
  int anchor;
  int hover;
};

struct AxisLimits {
  int min;
  int max;
  int site_size;
};

Span dragSpan(AxisDrag drag, AxisLimits limits) {
  int low = std::min(drag.anchor, drag.hover);
  int high = std::max(drag.anchor + limits.min - 1, drag.hover);
  if (high - low + 1 > limits.max) {
    if (drag.hover < drag.anchor) {
      low = high - limits.max + 1;
    } else {
      high = low + limits.max - 1;
    }
  }
  high = std::min(high, limits.site_size - 1);
  return {.start = low, .length = high - low + 1};
}

} // namespace

PendingPlacement dragRectangle(SiteLocation anchor, SiteLocation hover,
                               const FootprintLimits &limits,
                               const Site &site) {
  const Span x = dragSpan(
      {.anchor = anchor.x, .hover = hover.x},
      {.min = limits.min.w, .max = limits.max.w, .site_size = site.width});
  const Span y = dragSpan(
      {.anchor = anchor.y, .hover = hover.y},
      {.min = limits.min.h, .max = limits.max.h, .site_size = site.height});
  return {.origin = {.x = static_cast<uint8_t>(x.start),
                     .y = static_cast<uint8_t>(y.start)},
          .footprint = {.w = static_cast<uint8_t>(x.length),
                        .h = static_cast<uint8_t>(y.length)}};
}

flecs::entity buildingAt(flecs::entity site, SiteLocation cell) {
  flecs::entity found;
  site.children([&](flecs::entity child) {
    const auto *location = child.try_get<SiteLocation>();
    if (found.is_valid() || location == nullptr || !child.has<Building>()) {
      return;
    }
    const auto *footprint = child.try_get<Footprint>();
    const Footprint size = footprint != nullptr ? *footprint : Footprint{};
    if (cell.x >= location->x && cell.x < location->x + size.w &&
        cell.y >= location->y && cell.y < location->y + size.h) {
      found = child;
    }
  });
  return found;
}

PendingPlacement dragRun(SiteLocation anchor, SiteLocation hover) {
  const int dx = std::abs(hover.x - anchor.x);
  const int dy = std::abs(hover.y - anchor.y);
  if (dx >= dy) {
    return {.origin = {.x = std::min(anchor.x, hover.x), .y = anchor.y},
            .footprint = {.w = static_cast<uint8_t>(dx + 1), .h = 1}};
  }
  return {.origin = {.x = anchor.x, .y = std::min(anchor.y, hover.y)},
          .footprint = {.w = 1, .h = static_cast<uint8_t>(dy + 1)}};
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

PlacementResult canPlaceConnector(const OccupancyGrid &grid, int origin_x,
                                  int origin_y, Footprint footprint) {
  bool connected = false;
  for (int y = origin_y; y < origin_y + footprint.h; ++y) {
    for (int x = origin_x; x < origin_x + footprint.w; ++x) {
      if (!grid.inBounds(x, y)) {
        return PlacementResult::OutOfBounds;
      }
      if (grid.at(x, y) != CellContent::Empty) {
        return PlacementResult::Occupied;
      }
      connected = connected ||
                  hasNeighbour(grid, x, y, CellContent::Connector) ||
                  hasNeighbour(grid, x, y, CellContent::Building);
    }
  }
  const bool empty_site = std::ranges::all_of(
      grid.cells, [](CellContent c) { return c == CellContent::Empty; });
  return empty_site || connected ? PlacementResult::Ok
                                 : PlacementResult::ConnectorNotConnected;
}

} // namespace

PlacementResult canPlace(const OccupancyGrid &grid, PlacementKind kind,
                         SiteLocation origin, Footprint footprint) {
  if (kind == PlacementKind::Connector) {
    return canPlaceConnector(grid, origin.x, origin.y, footprint);
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
