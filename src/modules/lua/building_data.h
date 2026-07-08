/**
 * @file building_data.h
 * @brief Typed building/texture definitions parsed from a mod's buildings.lua.
 *
 * @details
 * A mod may ship a `buildings.lua` that `return`s an id-keyed map of building
 * prefab definitions (per ADR 011/012). parseBuildingData turns that table into
 * plain structs; applying the structs to the ECS lives separately (see
 * building_data.cpp) so the parse is pure and unit-testable without a flecs
 * world. Textures live in a separate textures.lua (see texture_data.h), loaded
 * first so building sprites can be validated against them.
 */
#pragma once

#include "modules/lua/mod_value.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// @brief A sprite clip rectangle into a texture atlas (pixels).
struct SpriteClipRect {
  int x;
  int y;
  uint32_t width;
  uint32_t height;
};

/// @brief A facility within a building definition (name + facility type tag).
struct FacilityDef {
  std::string name;
  /// One of "Launchpad" | "Office" | "Storage" | "Manufacturing".
  std::string type;
};

/// @brief A building prefab definition parsed from a mod's buildings.lua.
struct BuildingDef {
  std::string id;        ///< Stable registry id (the map key).
  std::string name;      ///< Display name / prefab entity name.
  std::string texture;   ///< Named texture the sprite clips from.
  SpriteClipRect rect{}; ///< Sprite clip rectangle into that texture.
  std::vector<FacilityDef> facilities;
  bool hidden = false; ///< ADR 011 §3: registry-only, no prefab created.
  // Deferred (ADR 009/012): `spawnable`, `buildable`, and `animations` are
  // carried through the deep merge but not yet mapped to the ECS - their
  // consumers (player build UI, animation rendering) do not exist yet, so we
  // leave them in the merged ModValue rather than adding unused struct fields.
};

/**
 * @brief Parse a merged buildings table (an id-keyed map) into definitions.
 *
 * Pure: reads the value tree only, performs no ECS work. Missing fields default
 * and malformed entries are skipped leniently; content validation and logging
 * happen at apply time (see validateBuildingDef).
 */
std::vector<BuildingDef> parseBuildingData(const ModValue &root);

/// @brief Whether `type` names a known facility component (see
/// addFacilityComponent). Kept in sync with that mapping.
bool isKnownFacilityType(const std::string &type);

/**
 * @brief Post-merge content check for a building definition.
 * @return nullopt if the definition can be turned into a prefab; otherwise a
 * human-readable reason it is broken (logged and skipped by the caller). A
 * `hidden` definition is intentionally suppressed, not broken, and is handled
 * separately by the caller.
 */
std::optional<std::string> validateBuildingDef(const BuildingDef &def);
