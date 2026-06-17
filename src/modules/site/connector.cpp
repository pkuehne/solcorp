#include "connector.h"
#include "modules/base/base.h"
#include "modules/engine/render.h"
#include "modules/lua/lua.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

static constexpr int TILE_SIZE = 32;
static constexpr int TILESET_COLS = 3;

static void renderLayer(const Renderer &renderer, flecs::entity tileset_e,
                        int src_col, int src_row, const Transform &transform,
                        double rotation) {
  if (!tileset_e.is_valid()) {
    return;
  }
  auto tex = tileset_e.get<Texture>();
  if (tex.ptr == nullptr) {
    spdlog::error("Connector tileset has null texture pointer");
    return;
  }
  renderTile(renderer, tex, src_col, src_row, TILE_SIZE, transform, rotation);
}

static void systemRenderConnector(flecs::entity e, const ConnectorTile &tile,
                                  const Transform &transform,
                                  const Renderer &renderer) {
  auto variant_idx = static_cast<int>(tile.variant);
  int src_col = variant_idx % TILESET_COLS;
  int src_row = variant_idx / TILESET_COLS;

  renderLayer(renderer, e.target<TilesetBase>(), src_col, src_row, transform,
              tile.rotation_deg);
  renderLayer(renderer, e.target<TilesetMarkings>(), src_col, src_row,
              transform, tile.rotation_deg);
  renderLayer(renderer, e.target<TilesetEmbellishments>(), src_col, src_row,
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

  world.system<const ConnectorTile, const Transform, const Renderer>(
           "Render Connectors")
      .kind(RenderPhase)
      .each(systemRenderConnector);
}
