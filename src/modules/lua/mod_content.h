/**
 * @file mod_content.h
 * @brief Loads each mod's data-driven content (textures, buildings, rockets,
 *        effects) into the ECS.
 *
 * @details
 * Mods describe their textures, building prefabs, rocket prefabs and effect
 * templates as plain data in `textures.lua` / `buildings.lua` / `rockets.lua` /
 * `effects.lua` (parsed by texture_data.h / building_data.h / rocket_data.h /
 * effect_data.h). loadModContent walks every loaded mod in dependency order,
 * reads those files, and applies them to the flecs world using the existing
 * prefab/texture helpers. Textures are applied before buildings so building
 * sprites resolve against already-loaded textures.
 */
#pragma once

#include "modules/engine/render.h" // Sprite
#include "modules/lua/building_data.h"
#include "modules/lua/effect_data.h"
#include "modules/lua/rocket_data.h"
#include "modules/lua/texture_data.h"
#include <flecs.h>
#include <string>
#include <vector>

/**
 * @brief Singleton tag requesting a reload of mod prefab content.
 *
 * Set it from any context, including a GUI handler running while the world is
 * in readonly mode (`world.add<ReloadPrefabsRequest>()`). A dedicated immediate
 * system consumes the tag and runs loadModContent outside of readonly mode,
 * where the structural changes it performs are legal.
 */
struct ReloadPrefabsRequest {};

/**
 * @brief Add the facility-type component named by `type` to `facility`.
 *
 * Maps a buildings.lua facility `type` string onto its ECS component
 * (Launchpad/Office/Storage/Manufacturing). Returns false (and logs) for an
 * unrecognised type, leaving the facility untouched.
 */
bool addFacilityComponent(flecs::entity facility, const std::string &type);

// Strong typedefs so the three string arguments to create_texture cannot be
// transposed at a call site (enforced by clang-tidy's swappable-parameters
// check).
struct TextureName {
  std::string value;
};
struct TextureFilename {
  std::string value;
};
struct TextureModName {
  std::string value;
};

/**
 * @brief Load `mods/<mod_name>/<filename>` as an SDL texture under Textures::.
 *
 * Rejects filenames containing "..". Idempotent: a repeat call reuses the
 * existing `Textures::<name>` entity and overwrites its Texture, so reloading
 * mod content does not create conflicting root entities.
 */
flecs::entity create_texture(flecs::world world, const TextureName &name,
                             const TextureFilename &filename,
                             const TextureModName &mod_name);

/// @brief Create (or reuse) an empty building prefab under Prefabs::Buildings.
flecs::entity create_building_prefab(const flecs::world &world,
                                     const std::string &name);

/// @brief Create (or reuse) a facility prefab child of `building`.
flecs::entity add_facility_to_building(const flecs::world &world,
                                       flecs::entity building,
                                       const std::string &name);

/// @brief Create (or reuse) a rocket prefab under Prefabs::Rockets.
flecs::entity create_rocket_prefab(const flecs::world &world,
                                   const std::string &name);

/**
 * @brief Build a Sprite that clips `rect` out of the named
 * `Textures::<texture>`.
 *
 * Returns an empty Sprite (and logs) if the texture does not exist.
 */
Sprite clip_sprite_from_texture(const flecs::world &world,
                                const std::string &texture,
                                SpriteClipRect rect);

/// @brief Load each parsed texture for `mod_name` into the world.
void applyTextureData(flecs::world &world, const std::string &mod_name,
                      const std::vector<TextureDef> &textures);

/// @brief Create a building prefab (sprite + facilities) for each definition.
void applyBuildingData(flecs::world &world,
                       const std::vector<BuildingDef> &buildings);

/// @brief Create a rocket prefab for each definition.
void applyRocketData(flecs::world &world,
                     const std::vector<RocketDef> &rockets);

/// @brief Create each effect once as a singleton entity under Effects (label +
/// modifier children). Sources attach to it later via HasEffect.
void applyEffectData(flecs::world &world,
                     const std::vector<EffectDef> &effects);

/**
 * @brief Read and apply every mod's textures.lua, buildings.lua then
 *        rockets.lua, in dependency load order. Each file is optional per mod.
 */
void loadModContent(flecs::world &world);
