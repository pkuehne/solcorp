/**
 * @file texture_data.h
 * @brief Typed texture definitions parsed from a mod's textures.lua.
 *
 * @details
 * A mod may ship a `textures.lua` that `return`s an id-keyed map of textures it
 * loads (`<name> -> { file = ... }`). parseTextureData turns that table into
 * plain structs; applying them to the ECS lives separately so the parse is pure
 * and unit-testable without a flecs world. Textures are loaded before buildings
 * so building sprites can be validated against them.
 */
#pragma once

#include "modules/lua/lua_data.h"
#include <string>
#include <vector>

/// @brief A named texture a mod loads.
struct TextureDef {
  std::string name; ///< Registry name (Textures::<name>), the map key.
  std::string file; ///< Filename relative to the mod directory.
};

/**
 * @brief Parse a textures.lua root table into typed definitions.
 *
 * Pure: reads the table only, performs no ECS work. Entries are keyed by
 * texture name; a missing `file` field defaults to empty (caught at apply
 * time).
 */
std::vector<TextureDef> parseTextureData(const LuaTableView &root);
