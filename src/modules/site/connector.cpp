#include "connector.h"
#include "modules/base/base.h"
#include "modules/engine/render.h"
#include "modules/lua/lua.h"
#include "modules/site/site.h"
#include <cstdint>
#include <flecs.h>
#include <unordered_map>
#include <utility>
#include <vector>

static constexpr int TILE_SIZE = 32;
static constexpr int TILESET_COLS = 3;

uint8_t rotateConnectorMaskCW(uint8_t mask) {
  // N(bit0)->E(bit1)->S(bit2)->W(bit3)->N: a left shift, with the top bit
  // wrapping back round to North.
  return static_cast<uint8_t>(((mask << 1) | (mask >> 3)) & 0xF);
}

namespace {
// Connections each variant makes at rotation_deg == 0. These must match the
// tilesheet artwork; Straight/DeadEnd are pinned by the existing Lua content,
// Corner/TJunction by the art's authored orientation. See ADR 010.
constexpr uint8_t STRAIGHT_BASE = NeighbourEast | NeighbourWest; // horizontal
constexpr uint8_t CORNER_BASE = NeighbourEast | NeighbourSouth;  // opens E+S
constexpr uint8_t TJUNCTION_BASE =
    NeighbourNorth | NeighbourEast | NeighbourSouth; // closed side faces West
constexpr uint8_t DEAD_END_BASE = NeighbourEast;     // stub opens East

// Smallest clockwise rotation (in degrees) that turns the base connection mask
// into the target mask. Both masks always have the same number of bits set.
double rotationToMatch(uint8_t base, uint8_t target) {
  for (int step = 0; step < 4; ++step) {
    if (base == target) {
      return step * 90.0;
    }
    base = rotateConnectorMaskCW(base);
  }
  return 0.0; // unreachable for well-formed connector masks
}
} // namespace

std::optional<ConnectorTiling> computeConnectorTiling(uint8_t neighbourMask) {
  const uint8_t mask = neighbourMask & 0xF;
  switch (__builtin_popcount(mask)) {
  case 0:
    // Isolated tile: keep whatever the author placed.
    return std::nullopt;
  case 1:
    return ConnectorTiling{.variant = ConnectorVariant::DeadEnd,
                           .rotation_deg =
                               rotationToMatch(DEAD_END_BASE, mask)};
  case 2: {
    const bool opposite = (mask == (NeighbourNorth | NeighbourSouth)) ||
                          (mask == (NeighbourEast | NeighbourWest));
    if (opposite) {
      return ConnectorTiling{.variant = ConnectorVariant::Straight,
                             .rotation_deg =
                                 rotationToMatch(STRAIGHT_BASE, mask)};
    }
    return ConnectorTiling{.variant = ConnectorVariant::Corner,
                           .rotation_deg = rotationToMatch(CORNER_BASE, mask)};
  }
  case 3:
    return ConnectorTiling{.variant = ConnectorVariant::TJunction,
                           .rotation_deg =
                               rotationToMatch(TJUNCTION_BASE, mask)};
  default: // 4 neighbours
    return ConnectorTiling{.variant = ConnectorVariant::Crossing,
                           .rotation_deg = 0.0};
  }
}

void systemRetileSiteConnectors(flecs::entity site) {
  auto key = [](int x, int y) { return (y << 8) | x; };

  std::unordered_map<int, flecs::entity> grid;
  std::vector<std::pair<flecs::entity, SiteLocation>> tiles;
  site.children([&](flecs::entity child) {
    if (child.has<ConnectorTile>() && child.has<SiteLocation>()) {
      const SiteLocation loc = child.get<SiteLocation>();
      grid[key(loc.x, loc.y)] = child;
      tiles.emplace_back(child, loc);
    }
  });

  auto occupied = [&](int x, int y) {
    if (x < 0 || y < 0) {
      return false;
    }
    return grid.find(key(x, y)) != grid.end();
  };

  for (auto &[e, loc] : tiles) {
    const int x = loc.x;
    const int y = loc.y;
    uint8_t mask = 0;
    if (occupied(x, y - 1)) {
      mask |= NeighbourNorth;
    }
    if (occupied(x + 1, y)) {
      mask |= NeighbourEast;
    }
    if (occupied(x, y + 1)) {
      mask |= NeighbourSouth;
    }
    if (occupied(x - 1, y)) {
      mask |= NeighbourWest;
    }

    const auto tiling = computeConnectorTiling(mask);
    if (!tiling.has_value()) {
      continue;
    }
    e.set<ConnectorTile>(
        {.variant = tiling->variant, .rotation_deg = tiling->rotation_deg});
  }
}

static void systemRenderConnector(flecs::entity e, const ConnectorTile &tile,
                                  const Transform &transform) {
  auto variant_idx = static_cast<int>(tile.variant);
  int src_col = variant_idx % TILESET_COLS;
  int src_row = variant_idx / TILESET_COLS;

  // Layers are drawn in order: base asphalt, then markings, then
  // embellishments.
  renderTileLayer<TilesetBase>(e, src_col, src_row, TILE_SIZE, transform,
                               tile.rotation_deg);
  renderTileLayer<TilesetMarkings>(e, src_col, src_row, TILE_SIZE, transform,
                                   tile.rotation_deg);
  renderTileLayer<TilesetEmbellishments>(e, src_col, src_row, TILE_SIZE,
                                         transform, tile.rotation_deg);
}

void registerConnectors(flecs::world &world) {
  world.component<ConnectorVariant>();
  world.component<ConnectorTile>()
      .member("variant", &ConnectorTile::variant)
      .member("rotation_deg", &ConnectorTile::rotation_deg);
  world.component<TilesetBase>();
  world.component<TilesetMarkings>();
  world.component<TilesetEmbellishments>();

  register_enum_table_lua(world, "ConnectorVariant", [](LuaEnumBuilder &b) {
    b.value("Straight", ConnectorVariant::Straight)
        .value("Corner", ConnectorVariant::Corner)
        .value("TJunction", ConnectorVariant::TJunction)
        .value("Crossing", ConnectorVariant::Crossing)
        .value("DeadEnd", ConnectorVariant::DeadEnd);
  });
  register_component_lua<ConnectorTile>(
      world, "ConnectorTile", [](LuaFieldBuilder<ConnectorTile> &b) {
        b.field<&ConnectorTile::variant>("variant")
            .field<&ConnectorTile::rotation_deg>("rotation_deg");
      });

  // Re-tile a site's connectors whenever its layout changed. Matches only
  // sites flagged SiteNeedsRelayout, so it does no work on a steady frame.
  // Registered before the scatter "Update Construction Sites" system (which
  // also keys off the tag and is responsible for clearing it), so this runs
  // first and must not remove the tag itself.
  world.system<Site>("Retile Connectors")
      .with<SiteNeedsRelayout>()
      .kind(ValidatePhase)
      .each(
          [](flecs::entity site, Site &) { systemRetileSiteConnectors(site); });

  world.system<const ConnectorTile, const Transform>("Render Connectors")
      .kind(RenderPhase)
      .each(systemRenderConnector);
}
