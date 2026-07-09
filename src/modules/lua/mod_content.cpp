#include "modules/lua/mod_content.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/engine/render.h"
#include "modules/lua/helpers.h"
#include "modules/lua/lua.h"
#include "modules/lua/lua_data.h"
#include "modules/lua/mod_registry.h"
#include "modules/rocket/rocket_module.h"
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

flecs::entity create_texture(flecs::world world, const TextureName &name,
                             const TextureFilename &filename,
                             const TextureModName &mod_name) {
  if (filename.value.find("..") != std::string::npos) {
    spdlog::error("Invalid filename {}", filename.value);
    return {};
  }
  auto location =
      (std::filesystem::path("mods") / mod_name.value / filename.value)
          .string();
  // Scope the lookup under Textures so a repeat call (e.g. reloading prefabs)
  // reuses the existing texture entity and overwrites its Texture, rather than
  // creating a conflicting root entity named the same.
  auto texture_node = world.entity("Textures");
  flecs::entity texture;
  world.scope(texture_node,
              [&] { texture = world.entity(name.value.c_str()); });
  return texture.set<Texture>(loadTexture(location, world));
  // TODO: Split this into Texture/TileSet/Sprite components so that neither
  // Texture nor Sprite needs to know about tile-sizes or columns
}

flecs::entity create_building_prefab(const flecs::world &world,
                                     const std::string &name) {
  auto buildings_node = world.lookup("Prefabs::Buildings");
  SC_ASSERT(buildings_node.is_valid(), "Prefabs::Buildings node not found");
  auto building_prefab = world.lookup("Prefabs::Core::Building");
  SC_ASSERT(building_prefab.is_valid(),
            "Prefabs::Core::Building prefab not found");
  return create_prefab_under(world, buildings_node, building_prefab, name);
}

flecs::entity add_facility_to_building(const flecs::world &world,
                                       flecs::entity building,
                                       const std::string &name) {
  auto facility_prefab = world.lookup("Prefabs::Core::Facility");
  SC_ASSERT(facility_prefab.is_valid(),
            "Prefabs::Core::Facility prefab not found");
  return create_prefab_under(world, building, facility_prefab, name);
}

flecs::entity create_rocket_prefab(const flecs::world &world,
                                   const std::string &name) {
  auto rockets_node = world.lookup("Prefabs::Rockets");
  SC_ASSERT(rockets_node.is_valid(), "Prefabs::Rockets node not found");
  auto rocket_prefab = world.lookup("Prefabs::Core::Rocket");
  SC_ASSERT(rocket_prefab.is_valid(), "Prefabs::Core::Rocket prefab not found");
  return create_prefab_under(world, rockets_node, rocket_prefab, name);
}

Sprite clip_sprite_from_texture(const flecs::world &world,
                                const std::string &texture,
                                SpriteClipRect rect) {
  std::string texture_name("Textures::");
  texture_name.append(texture);
  auto textureE = world.lookup(texture_name.c_str());
  if (!textureE.is_valid()) {
    spdlog::error("Texture {} does not exist", texture);
    return {};
  }
  Sprite sprite;
  sprite.texture = textureE;
  sprite.x = rect.x;
  sprite.y = rect.y;
  sprite.width = rect.width;
  sprite.height = rect.height;
  return sprite;
}

void applyTextureData(flecs::world &world,
                      const std::vector<TextureDef> &textures) {
  for (const auto &texture : textures) {
    create_texture(world, TextureName{texture.name},
                   TextureFilename{texture.file}, TextureModName{texture.mod});
  }
}

void applyBuildingData(flecs::world &world,
                       const std::vector<BuildingDef> &buildings) {
  for (const auto &building : buildings) {
    auto prefab = create_building_prefab(world, building.id);
    prefab.set<Label>({building.name});

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

void applyRocketData(flecs::world &world,
                     const std::vector<RocketDef> &rockets) {
  for (const auto &rocket : rockets) {
    auto rocket_prefab = create_rocket_prefab(world, rocket.id);
    rocket_prefab.set<Label>({rocket.name});

    // Override the cost stat inherited from Prefabs::Core::Rocket with the
    // data-driven value. The other Rocket stats are not data-driven yet, so
    // they keep their Core defaults.
    Rocket stats;
    stats.cost.setBase(static_cast<double>(rocket.cost));
    rocket_prefab.set<Rocket>(stats);

    for (const auto &[orbit_name, max_mass] : rocket.target_orbits) {
      auto orbit = world.lookup(orbit_name.c_str());
      if (!orbit.is_valid()) {
        // A broken orbit reference is recoverable (ADR 011 §5): drop this
        // lift capability and keep the rocket, rather than aborting the load.
        spdlog::error("Rocket '{}' references unknown orbit '{}'; skipping it",
                      rocket.id, orbit_name);
        continue;
      }
      rocket_prefab.set<CanLiftTo>(orbit, {max_mass});
    }
  }
}

void applyEffectData(flecs::world &world,
                     const std::vector<EffectDef> &effects) {
  auto effects_node = world.lookup("Effects");
  SC_ASSERT(effects_node.is_valid(), "Effects node not found");

  for (const auto &effect : effects) {
    // An effect exists exactly once: create (or reuse, on reload) a single
    // entity under Effects keyed by id. Sources reference it via HasEffect.
    flecs::entity effect_entity;
    world.scope(effects_node,
                [&] { effect_entity = world.entity(effect.id.c_str()); });
    effect_entity.add<Effect>().set<Label>({effect.name});

    // The modifiers are children of the effect entity (applyModifiers walks an
    // effect's children for Modifier). Name them by index so a reload reuses
    // the same children rather than appending duplicates.
    int index = 0;
    for (const auto &modifier : effect.modifiers) {
      auto modifier_name = fmt::format("Modifier {}", index++);
      flecs::entity modifier_entity;
      world.scope(effect_entity, [&] {
        modifier_entity = world.entity(modifier_name.c_str());
      });
      modifier_entity.set<Modifier>(modifier);
    }
  }
}

namespace {

/// @brief Fold `mods/<mod.id>/<filename>` (if present) into the registry under
/// `category`, materialising it so it can be deep-merged across mods.
void mergeModDataFile(ModRegistry &registry, const std::string &category,
                      const Mod &mod, const char *filename) {
  std::filesystem::path path =
      std::filesystem::path("mods") / mod.id / filename;
  if (!std::filesystem::exists(path)) {
    return;
  }
  LuaDataFile data(path.string());
  if (!data.ok()) {
    spdlog::error("Failed to load {}: {}", path.string(), data.error());
    return;
  }
  registry.merge(category, data.materialize(), mod.id, mod.load_order);
}

/// @brief Turn the merged buildings table into the definitions that should
/// become prefabs: hidden entries are suppressed, broken entries are logged
/// (with provenance) and skipped (ADR 011 §3/§5).
std::vector<BuildingDef> selectBuildingPrefabs(const ModRegistry &registry) {
  std::vector<BuildingDef> prefabs;
  for (const BuildingDef &def :
       parseBuildingData(registry.merged("buildings"))) {
    if (def.hidden) {
      spdlog::debug("Building '{}' is hidden; no prefab created", def.id);
      continue;
    }
    if (auto reason = validateBuildingDef(def)) {
      spdlog::error("Skipping invalid building '{}': {} (defined by {})",
                    def.id, *reason,
                    registry.historyString("buildings", def.id));
      continue;
    }
    prefabs.push_back(def);
  }
  return prefabs;
}

/// @brief Turn the merged textures table into the textures to load: each
/// texture's file is resolved against the mod that last supplied it (merge
/// provenance), and an entry with no file is logged and skipped (ADR 011 §5).
std::vector<TextureDef> selectTextures(const ModRegistry &registry) {
  std::vector<TextureDef> textures;
  for (TextureDef def : parseTextureData(registry.merged("textures"))) {
    if (def.file.empty()) {
      spdlog::error("Skipping texture '{}': it has no file (defined by {})",
                    def.name, registry.historyString("textures", def.name));
      continue;
    }
    def.mod = registry.lastSource("textures", def.name);
    textures.push_back(def);
  }
  return textures;
}

/// @brief Turn the merged effects table into the effects to instantiate: an
/// effect with no modifiers does nothing, so it is logged (with provenance) and
/// skipped (ADR 011 §5).
std::vector<EffectDef> selectEffects(const ModRegistry &registry) {
  std::vector<EffectDef> effects;
  for (const EffectDef &def : parseEffectData(registry.merged("effects"))) {
    if (def.modifiers.empty()) {
      spdlog::error("Skipping effect '{}': it has no modifiers (defined by {})",
                    def.id, registry.historyString("effects", def.id));
      continue;
    }
    effects.push_back(def);
  }
  return effects;
}

/// @brief Tag each created prefab under `node` with the mod chain that produced
/// its definition (ADR 011 §6), so the developer UI can show provenance. `ids`
/// are the definitions that survived selection (their prefab entity names).
void recordProvenance(const flecs::world &world, const ModRegistry &registry,
                      const std::string &category, const char *node,
                      const std::vector<std::string> &ids) {
  auto parent = world.lookup(node);
  if (!parent.is_valid()) {
    return;
  }
  for (const std::string &id : ids) {
    auto prefab = parent.lookup(id.c_str());
    if (prefab.is_valid()) {
      prefab.set<PrefabProvenance>({registry.historyChain(category, id)});
    }
  }
}

/// @brief The `.id` of every definition in `defs`, preserving order.
template <typename Def>
std::vector<std::string> defIds(const std::vector<Def> &defs) {
  std::vector<std::string> ids;
  ids.reserve(defs.size());
  for (const Def &def : defs) {
    ids.push_back(def.id);
  }
  return ids;
}

} // namespace

void loadModContent(flecs::world &world) {
  world.component<PrefabProvenance>();

  // Suspend flecs command deferral for the duration of the load. We create a
  // texture and then immediately look it up by name when clipping building
  // sprites; while deferred, the entity's name/child_of edges are queued and
  // not yet visible to lookups within the same system run, so the lookup
  // would fail. Applying structurally and immediately keeps
  // create-then-lookup consistent. Resumed (and any queued commands restored)
  // afterwards.
  bool was_deferred = world.is_deferred();
  if (was_deferred) {
    world.defer_suspend();
  }

  // Every mod's data files are deep-merged across mods (ADR 011): fold each
  // file into the registry in resolved load order, then validate and apply the
  // merged result once per category, so a later mod can patch or suppress an
  // earlier entry rather than blindly overwriting a same-named prefab.
  ModRegistry registry;
  for_each_mod(world, [&](const Mod &mod) {
    mergeModDataFile(registry, "textures", mod, "textures.lua");
    mergeModDataFile(registry, "buildings", mod, "buildings.lua");
    mergeModDataFile(registry, "rockets", mod, "rockets.lua");
    mergeModDataFile(registry, "effects", mod, "effects.lua");
  });

  // Textures load before buildings so a building sprite resolves against any
  // mod's texture (and a later mod can override a texture another mod uses).
  auto buildings = selectBuildingPrefabs(registry);
  auto rockets = parseRocketData(registry.merged("rockets"));
  applyTextureData(world, selectTextures(registry));
  applyBuildingData(world, buildings);
  applyRocketData(world, rockets);
  applyEffectData(world, selectEffects(registry));

  // Record each prefab's mod-origin chain for the developer window (ADR 011
  // §6).
  recordProvenance(world, registry, "buildings", "Prefabs::Buildings",
                   defIds(buildings));
  recordProvenance(world, registry, "rockets", "Prefabs::Rockets",
                   defIds(rockets));

  if (was_deferred) {
    world.defer_resume();
  }
}
