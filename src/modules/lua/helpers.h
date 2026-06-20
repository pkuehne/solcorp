#pragma once

#include "modules/site/connector.h"
#include "modules/stats/stats.h"
#include <flecs.h>
#include <lua.hpp>
#include <string>
#include <vector>

struct RocketName {
  std::string value;
};
struct RocketPrefabType {
  std::string value;
};

void load_helpers_namespace(lua_State *L);

/**
 * @brief Create (or reuse, if it already exists) a named prefab under `parent`.
 *
 * Scoping the creation makes the name resolve relative to `parent`, so a repeat
 * call with the same name returns the existing prefab instead of creating a
 * conflicting root entity. This keeps prefab registration idempotent, which the
 * developer "Reload Prefabs" path relies on.
 */
flecs::entity create_prefab_under(const flecs::world &world,
                                  flecs::entity parent, flecs::entity base,
                                  const std::string &name);

flecs::entity create_site(const flecs::world &world, const std::string &name,
                          uint8_t width, uint8_t height,
                          bool make_current = false);
flecs::entity create_rocket(const flecs::world &world, const RocketName &name,
                            const RocketPrefabType &prefab,
                            flecs::entity parent = flecs::entity());
flecs::entity create_building(flecs::world world, const std::string &name,
                              const std::string &prefab, uint8_t x, uint8_t y,
                              flecs::entity site);
flecs::entity create_effect(const flecs::world &world, const std::string &name,
                            flecs::entity source);
flecs::entity add_modifier(const flecs::world &world, flecs::entity effect,
                           const Modifier &mod);
flecs::entity create_contract(flecs::world &world, const std::string &name,
                              const std::string &client,
                              const std::string &description,
                              uint32_t upfront_payment,
                              uint32_t completion_payment);
flecs::entity create_contract_payload(const flecs::world &world,
                                      flecs::entity contract,
                                      const std::string &name, uint32_t mass,
                                      const std::string &target_orbit_name);
std::vector<flecs::entity> get_all_contracts(const flecs::world &world);
std::vector<flecs::entity> get_all_active_contracts(const flecs::world &world);
flecs::entity create_connector(flecs::world &world, const std::string &name,
                               ConnectorVariant variant, double rotation,
                               uint8_t x, uint8_t y, flecs::entity site,
                               const std::string &base_tileset,
                               const std::string &markings_tileset);
