#pragma once
#include <cstdint>
#include <flecs.h>

enum class ConnectorVariant : uint8_t {
  Straight  = 0,
  Corner    = 1,
  TJunction = 2,
  Crossing  = 3,
  DeadEnd   = 4,
};

struct TilesetBase {};
struct TilesetMarkings {};
struct TilesetEmbellishments {};

struct ConnectorTile {
  ConnectorVariant variant = ConnectorVariant::Straight;
  double rotation_deg = 0.0;
};

void registerConnectors(flecs::world &world);
