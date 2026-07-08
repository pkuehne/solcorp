/**
 * @file rocket_data.h
 * @brief Typed rocket definitions parsed from a mod's rockets.lua.
 *
 * @details
 * A mod may ship a `rockets.lua` that `return`s an id-keyed map of rockets it
 * loads. Each entry has a stable `id` (the map key, used as the prefab entity
 * name) and an optional display `name` (player-visible, applied as a Label).
 * parseRocketData turns that table into plain structs; applying them to the ECS
 * lives separately so the parse is pure and unit-testable without a flecs
 * world.
 */
#pragma once

#include "modules/lua/mod_value.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

/// @brief A named rocket a mod loads.
struct RocketDef {
  std::string id;   ///< Stable registry id (the map key, prefab entity name).
  std::string name; ///< Display name (player-visible Label).
  int cost;         ///< Cost of the rocket.
  std::map<std::string, uint32_t>
      target_orbits; ///< The mass that can be lifted to each orbit
};

/**
 * @brief Parse a rockets.lua root table (an id-keyed map) into definitions.
 *
 * Pure: reads the table only, performs no ECS work. Each entry is keyed by the
 * rocket's stable id; a missing `name` defaults to the id, a missing `cost`
 * defaults to 0 and a malformed `orbits` entry is skipped leniently. Content
 * validation (e.g. that an orbit exists) happens at apply time.
 */
std::vector<RocketDef> parseRocketData(const ModValue &root);
