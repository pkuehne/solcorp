/**
 * @file effect_data.h
 * @brief Typed effect template definitions parsed from a mod's effects.lua.
 *
 * @details
 * A mod may ship an `effects.lua` that `return`s an id-keyed map of effects it
 * loads. Each entry has a stable `id` (the map key, used as the effect entity
 * name under Effects), an optional display `name` (player-visible, applied as a
 * Label) and the `modifiers` it carries. Modifiers are part of an effect and
 * have no separate definition file. parseEffectData turns that table into plain
 * structs; applying them to the ECS lives separately so the parse is pure and
 * unit-testable without a flecs world.
 */
#pragma once

#include "modules/lua/mod_value.h"
#include "modules/stats/stats.h" // Modifier
#include <string>
#include <vector>

/// @brief A named effect a mod loads.
struct EffectDef {
  std::string id;   ///< Stable registry id (the map key, effect entity name).
  std::string name; ///< Display name (player-visible Label).
  std::vector<Modifier> modifiers; ///< The modifiers this effect applies.
};

/**
 * @brief Parse an effects.lua root table (an id-keyed map) into definitions.
 *
 * Pure: reads the table only, performs no ECS work. Each entry is keyed by the
 * effect's stable id; a missing `name` defaults to the id and a malformed
 * `modifiers` entry is skipped leniently. A modifier defaults its additive to 0
 * and multiplicative to 1 when absent.
 */
std::vector<EffectDef> parseEffectData(const ModValue &root);
