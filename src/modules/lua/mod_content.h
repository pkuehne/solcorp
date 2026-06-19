/**
 * @file mod_content.h
 * @brief Loads each mod's data-driven content (textures, buildings) into the
 *        ECS.
 *
 * @details
 * Mods describe their textures and building prefabs as plain data in
 * `textures.lua` / `buildings.lua` (parsed by texture_data.h /
 * building_data.h). loadModContent walks every loaded mod in dependency order,
 * reads those files, and applies them to the flecs world using the existing
 * prefab/texture helpers. Textures are applied before buildings so building
 * sprites resolve against already-loaded textures.
 */
#pragma once

#include "modules/lua/building_data.h"
#include "modules/lua/texture_data.h"
#include <flecs.h>
#include <string>
#include <vector>

/**
 * @brief Add the facility-type component named by `type` to `facility`.
 *
 * Maps a buildings.lua facility `type` string onto its ECS component
 * (Launchpad/Office/Storage/Manufacturing). Returns false (and logs) for an
 * unrecognised type, leaving the facility untouched.
 */
bool addFacilityComponent(flecs::entity facility, const std::string &type);

/// @brief Load each parsed texture for `mod_name` into the world.
void applyTextureData(flecs::world &world, const std::string &mod_name,
                      const std::vector<TextureDef> &textures);

/// @brief Create a building prefab (sprite + facilities) for each definition.
void applyBuildingData(flecs::world &world,
                       const std::vector<BuildingDef> &buildings);

/**
 * @brief Read and apply every mod's textures.lua then buildings.lua, in
 *        dependency load order. Both files are optional per mod.
 */
void loadModContent(flecs::world &world);
