#pragma once
#include <cstdint>
#include <flecs.h>
#include <optional>

enum class ConnectorVariant : uint8_t {
  Straight = 0,
  Corner = 1,
  TJunction = 2,
  Crossing = 3,
  DeadEnd = 4,
};

struct TilesetBase {};
struct TilesetMarkings {};
struct TilesetEmbellishments {};

struct ConnectorTile {
  ConnectorVariant variant = ConnectorVariant::Straight;
  double rotation_deg = 0.0;
};

/// @brief Bitmask of which orthogonal neighbours of a connector are also
/// connectors. Bits follow the clockwise rotation order used by the tilesheet.
enum ConnectorNeighbour : uint8_t {
  NeighbourNorth = 1u << 0,
  NeighbourEast = 1u << 1,
  NeighbourSouth = 1u << 2,
  NeighbourWest = 1u << 3,
};

/// @brief The variant + rotation a connector should render with for a given
/// neighbour configuration.
struct ConnectorTiling {
  ConnectorVariant variant;
  double rotation_deg;
};

/// @brief Rotate a 4-bit neighbour mask 90 degrees clockwise (N->E->S->W).
uint8_t rotateConnectorMaskCW(uint8_t mask);

/// @brief Pick the variant and rotation for a connector from its neighbour
/// mask. Returns std::nullopt for an isolated tile (mask == 0); callers should
/// leave such a tile's authored variant/rotation untouched. See ADR 010.
std::optional<ConnectorTiling> computeConnectorTiling(uint8_t neighbourMask);

/// @brief Recompute the variant + rotation of every connector tile belonging to
/// a site, from which orthogonal neighbours are also connectors. Declared here
/// so it can be driven directly in tests.
void systemRetileSiteConnectors(flecs::entity site);

void registerConnectors(flecs::world &world);
