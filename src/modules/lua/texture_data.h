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

#include "modules/lua/mod_value.h"
#include <string>
#include <vector>

/// @brief A named texture a mod loads.
struct TextureDef {
  std::string name; ///< Registry name (Textures::<name>), the map key.
  std::string file; ///< Filename relative to the mod directory.
  std::string mod;  ///< Owning mod id (dir the `file` is relative to). Filled
                    ///< from provenance at apply time, not by the parser.
};

/**
 * @brief Parse a merged textures table into typed definitions.
 *
 * Pure: reads the value tree only, performs no ECS work. Entries are keyed by
 * texture name; a missing `file` field defaults to empty (caught at apply
 * time). `mod` is left empty here; the caller fills it from merge provenance.
 */
std::vector<TextureDef> parseTextureData(const ModValue &root);
