#include "modules/lua/mod_content.h"
#include "modules/lua/helpers.h"
#include "modules/lua/lua.h"
#include "modules/lua/lua_data.h"
#include "modules/lua/lua_registry.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"

#include <filesystem>
#include <functional>

bool addFacilityComponent(flecs::entity facility, const std::string &type) {
  if (type == "Launchpad") {
    facility.add<Launchpad>();
  } else if (type == "Office") {
    facility.add<Office>();
  } else if (type == "Storage") {
    facility.add<Storage>();
  } else if (type == "Manufacturing") {
    facility.add<Manufacturing>();
  } else {
    spdlog::error("Unknown facility type '{}' on facility {}", type,
                  facility.name().c_str());
    return false;
  }
  return true;
}

void applyTextureData(flecs::world &world, const std::string &mod_name,
                      const std::vector<TextureDef> &textures) {
  for (const auto &texture : textures) {
    create_texture(world, TextureName{texture.name},
                   TextureFilename{texture.file}, TextureModName{mod_name});
  }
}

void applyBuildingData(flecs::world &world,
                       const std::vector<BuildingDef> &buildings) {
  for (const auto &building : buildings) {
    auto prefab = create_building_prefab(world, building.name);

    if (!building.texture.empty()) {
      Sprite sprite =
          clip_sprite_from_texture(world, building.texture, building.rect);
      prefab.set<Sprite>(sprite);
    }

    for (const auto &facility : building.facilities) {
      auto facility_prefab =
          add_facility_to_building(world, prefab, facility.name);
      addFacilityComponent(facility_prefab, facility.type);
    }
  }
}

namespace {

/// @brief Read `mods/<mod_name>/<filename>` (if present) and hand its root
/// table to `apply`. Logs and skips a file that fails to load.
void readModDataFile(const std::string &mod_name, const char *filename,
                     const std::function<void(const LuaTableView &)> &apply) {
  std::filesystem::path path =
      std::filesystem::path("mods") / mod_name / filename;
  if (!std::filesystem::exists(path)) {
    return;
  }
  LuaDataFile data(path.string());
  if (!data.ok()) {
    spdlog::error("Failed to load {}: {}", path.string(), data.error());
    return;
  }
  apply(data.root());
}

} // namespace

void loadModContent(flecs::world &world) {
  // Suspend flecs command deferral for the duration of the load. We create a
  // texture and then immediately look it up by name when clipping building
  // sprites; while deferred, the entity's name/child_of edges are queued and
  // not yet visible to lookups within the same system run, so the lookup would
  // fail. Applying structurally and immediately keeps create-then-lookup
  // consistent. Resumed (and any queued commands restored) afterwards.
  bool was_deferred = world.is_deferred();
  if (was_deferred) {
    world.defer_suspend();
  }

  // Two passes, each visiting mods in resolved dependency order (later mods win
  // on name conflicts). Pass 1 loads every mod's textures before pass 2 clips
  // any building sprite, so a building resolves any texture regardless of which
  // mod owns it and a later mod can override a texture another mod's building
  // uses.
  run_on_every_mod(world, [&world](lua_State *L) {
    std::string mod_name = lua_get_mod_name(L);
    readModDataFile(mod_name, "textures.lua", [&](const LuaTableView &root) {
      applyTextureData(world, mod_name, parseTextureData(root));
    });
  });

  run_on_every_mod(world, [&world](lua_State *L) {
    std::string mod_name = lua_get_mod_name(L);
    readModDataFile(mod_name, "buildings.lua", [&](const LuaTableView &root) {
      applyBuildingData(world, parseBuildingData(root));
    });
  });

  if (was_deferred) {
    world.defer_resume();
  }
}
