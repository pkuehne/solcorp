#pragma once

#include <cstdint>
#include <flecs.h>
#include <vector>

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

/// @brief Builds the occupancy grid of @p site from its children: buildings
/// fill their Footprint (1x1 if they have none), connectors fill one cell
OccupancyGrid buildOccupancyGrid(flecs::entity site);
