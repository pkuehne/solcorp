#pragma once

#include "site.h"
#include <cstdint>
#include <flecs.h>
#include <string>
#include <vector>

/// @brief A category of prefab the player may build on a site.
/// site --Buildable--> Prefabs::<category> (ADR 009 §1)
struct Buildable {};

/// @brief Marks a prefab as a single 1x1 object placed by one click, rather
/// than a drag-sized multi-tile building (ADR 009 §2)
struct FixedSize {};

/// @brief The realised size of a placed instance; its origin is its
/// SiteLocation (ADR 009 §2)
struct Footprint {
  uint8_t w = 1;
  uint8_t h = 1;
};

enum class CellContent : uint8_t { Empty, Building, Connector };

/// @brief What occupies each cell of a site (ADR 009 §5)
struct OccupancyGrid {
  uint8_t width = 0;
  uint8_t height = 0;
  std::vector<CellContent> cells; // row-major

  [[nodiscard]] bool inBounds(int x, int y) const;
  /// @returns CellContent::Empty for out-of-bounds cells
  [[nodiscard]] CellContent at(int x, int y) const;
};

enum class PlacementKind : uint8_t { Building, Connector };

enum class PlacementResult : uint8_t {
  Ok,
  OutOfBounds,
  Occupied,
  TooSmall,
  TooLarge,
  NotConnectedToRoad,
  ConnectorNotConnected,
};

/// @brief Whether something of @p kind may cover @p footprint from @p origin
///
/// Every cell must be in bounds and empty. A building needs one cell
/// orthogonally next to a connector; a connector run needs one cell next to a
/// connector or building, unless the site is empty.
PlacementResult canPlace(const OccupancyGrid &grid, PlacementKind kind,
                         SiteLocation origin, Footprint footprint);

/// @brief Empty for PlacementResult::Ok
std::string placementMessage(PlacementResult result);

/// @brief Smallest footprint of a draggable building (ADR 009 §2)
constexpr Footprint MIN_BUILDING_FOOTPRINT{.w = 2, .h = 2};

/// @brief Size range a building definition allows, per dimension. A minimum
/// below MIN_BUILDING_FOOTPRINT is raised to it; the site bounds the maximum
/// separately (canPlace reports OutOfBounds).
struct FootprintLimits {
  Footprint min = MIN_BUILDING_FOOTPRINT;
  Footprint max{.w = UINT8_MAX, .h = UINT8_MAX};
};

/// @brief Whether a draggable building of @p footprint fits @p limits
/// (FixedSize buildings are always 1x1 and are not size-checked)
PlacementResult checkFootprintSize(Footprint footprint,
                                   const FootprintLimits &limits);

/// @brief How dragging sizes a placement: not at all, as a rectangle, or as a
/// one-cell-wide run
enum class DragShape : uint8_t { None, Rectangle, Run };

/// @brief Run for connectors, None for fixed-size buildings, else Rectangle
DragShape dragShape(flecs::entity prefab);

/// @brief The footprint a click places for @p prefab: 1x1 if FixedSize,
/// otherwise its definition's minimum, raised to MIN_BUILDING_FOOTPRINT
Footprint placementFootprint(flecs::entity prefab);

/// @brief The sizes build mode allows for @p prefab: 1x1 if FixedSize,
/// otherwise from placementFootprint up to its definition's maximum
FootprintLimits placementLimits(flecs::entity prefab);

struct PendingPlacement {
  SiteLocation origin;
  Footprint footprint;
};

/// @brief The rectangle dragged from @p anchor to @p hover: the minimum-size
/// rectangle at @p anchor, stretched to include @p hover. Past the maximum the
/// dragged edge is held back; the result is then cut at the @p site edge,
/// which can leave it below the minimum
PendingPlacement dragRectangle(SiteLocation anchor, SiteLocation hover,
                               const FootprintLimits &limits, const Site &site);

/// @brief The building of @p site whose footprint covers @p cell; invalid if
/// none does. A building without a Footprint covers one cell
flecs::entity buildingAt(flecs::entity site, SiteLocation cell);

/// @brief The straight run from @p anchor towards @p hover, along whichever
/// axis the mouse has moved further (horizontal on a tie)
PendingPlacement dragRun(SiteLocation anchor, SiteLocation hover);

/// @brief Builds the occupancy grid of @p site from its children: buildings
/// fill their Footprint (1x1 if they have none), connectors fill one cell
OccupancyGrid buildOccupancyGrid(flecs::entity site);
