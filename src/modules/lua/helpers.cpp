#include "helpers.h"
#include "modules/base/assert.h"
#include "modules/base/notification.h"
#include "modules/engine/render.h"
#include "modules/lua/entity.h"
#include "modules/lua/lua_registry.h"
#include "modules/site/connector.h"
#include "modules/site/helpers.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include <flecs.h>
#include <modules/rocket/rocket_module.h>

flecs::entity create_site(const flecs::world &world, const std::string &name,
                          uint8_t width, uint8_t height, bool make_current) {
  auto site = world.entity(name.c_str())
                  .set<Site>({.width = width, .height = height})
                  .set<Transform>({.relativePosition = {.x = 340, .y = 240},
                                   .worldPosition = {}})
                  .add<SiteNeedsRelayout>();
  if (make_current) {
    site.add<CurrentSite>();
  }
  auto earth = world.lookup("Sun::Earth");
  SC_ASSERT(earth.is_valid(), "Sun::Earth entity not found");
  site.child_of(earth);
  return site;
}

flecs::entity create_prefab_under(const flecs::world &world,
                                  flecs::entity parent, flecs::entity base,
                                  const std::string &name) {
  flecs::entity prefab;
  world.scope(parent, [&] { prefab = world.prefab(name.c_str()).is_a(base); });
  return prefab;
}

flecs::entity create_rocket_prefab(const flecs::world &world,
                                   const std::string &name) {
  auto rockets_node = world.lookup("Prefabs::Rockets");
  SC_ASSERT(rockets_node.is_valid(), "Prefabs::Rockets node not found");
  auto rocket_prefab = world.lookup("Prefabs::Core::Rocket");
  SC_ASSERT(rocket_prefab.is_valid(), "Prefabs::Core::Rocket prefab not found");
  return create_prefab_under(world, rockets_node, rocket_prefab, name);
}

flecs::entity create_rocket(const flecs::world &world, const RocketName &name,
                            const RocketPrefabType &prefab,
                            flecs::entity parent) {
  std::string prefab_name = "Prefabs::Rockets::";
  prefab_name.append(prefab.value);
  auto prefab_entity = world.lookup(prefab_name.c_str());
  if (!prefab_entity.is_valid()) {
    spdlog::error("Rocket prefab {} does not exist", prefab_name);
    return {};
  }
  auto rocket = world.entity(name.value.c_str()).is_a(prefab_entity);
  if (parent.is_valid()) {
    rocket.child_of(parent);
  }
  return rocket;
}

flecs::entity create_building(flecs::world world, const std::string &name,
                              const std::string &prefab, uint8_t x, uint8_t y,
                              flecs::entity site) {
  return instantiateBuilding(world, BuildingName{name}, BuildingPrefab{prefab},
                             SiteLocation{.x = x, .y = y}, site);
}

flecs::entity add_target_orbit_to_rocket(const flecs::world &world,
                                         flecs::entity rocket,
                                         const std::string &orbit_name,
                                         uint32_t max_mass) {
  auto orbit = world.lookup(orbit_name.c_str());
  SC_ASSERT(orbit.is_valid(), fmt::format("Orbit {} not found", orbit_name));
  rocket.set<CanLiftTo>(orbit, {max_mass});
  return rocket;
}

flecs::entity create_effect(const flecs::world &world, const std::string &name,
                            flecs::entity source) {
  auto effect = world.entity(name.c_str())
                    .add<Effect>()
                    .child_of(world.lookup("Effects"));
  if (source.is_valid()) {
    source.add<HasEffect>(effect);
  }
  return effect;
}

flecs::entity add_modifier(const flecs::world &world, flecs::entity effect,
                           const Modifier &mod) {
  if (!effect.is_valid()) {
    return {};
  }
  return world.entity().child_of(effect).set<Modifier>(mod);
}

flecs::entity create_contract(flecs::world &world, const std::string &name,
                              const std::string &client,
                              const std::string &description,
                              uint32_t upfront_payment,
                              uint32_t completion_payment) {
  auto contracts_node = world.lookup("Contracts");
  SC_ASSERT(contracts_node.is_valid(), "Contracts node not found");
  instantiateNotification(world, "Contract Created",
                          fmt::format("A new contract '{}' is available", name),
                          world.lookup("NotificationCategories::Contracts"),
                          NotificationSeverity::Medium);
  auto entity_name = fmt::format("Contract {}", Contract::max_id++);
  return world.entity(entity_name.c_str())
      .set<Contract>({.name = name,
                      .client = client,
                      .description = description,
                      .upfront_payment = upfront_payment,
                      .completion_payment = completion_payment})
      .add<ContractCurrentState>(world.lookup("States::Contract::Open"))
      .child_of(contracts_node);
}

flecs::entity create_contract_payload(const flecs::world &world,
                                      flecs::entity contract,
                                      const std::string &name, uint32_t mass,
                                      const std::string &target_orbit_name) {
  if (!contract.is_valid()) {
    spdlog::warn("Cannot add payload to invalid contract");
    return flecs::entity{};
  }
  if (contract.has<ContractPayload>(flecs::Wildcard)) {
    spdlog::warn("Contract {} already has a payload, rejecting",
                 contract.get<Contract>().name.c_str());
    return flecs::entity{};
  }
  auto payload_entity =
      world.entity(name.c_str()).set<Payload>({mass}).child_of(contract);
  contract.add<ContractPayload>(payload_entity);
  if (!target_orbit_name.empty()) {
    auto orbit_entity = world.lookup(target_orbit_name.c_str());
    if (orbit_entity.is_valid()) {
      contract.add<ContractTargetOrbit>(orbit_entity);
    } else {
      spdlog::error("Orbit {} not found", target_orbit_name);
    }
  }
  return payload_entity;
}

std::vector<flecs::entity> get_all_contracts(const flecs::world &world) {
  std::vector<flecs::entity> result;
  world.query_builder<Contract>().build().each(
      [&](flecs::entity e, Contract &) { result.push_back(e); });
  return result;
}

flecs::entity create_connector(flecs::world &world, const std::string &name,
                               ConnectorVariant variant, double rotation,
                               uint8_t x, uint8_t y, flecs::entity site,
                               const std::string &base_tileset,
                               const std::string &markings_tileset) {
  if (!site.is_valid()) {
    spdlog::error("Cannot create connector: site entity is not valid");
    return {};
  }

  Transform t;
  t.relativePosition.x = static_cast<float>(x) * 32;
  t.relativePosition.y = static_cast<float>(y) * 32;

  auto e =
      world.entity(name.c_str())
          .set<ConnectorTile>({.variant = variant, .rotation_deg = rotation})
          .set<SiteLocation>({.x = x, .y = y})
          .set<Transform>(t)
          .child_of(site);

  auto base_e = world.lookup(("Textures::" + base_tileset).c_str());
  if (!base_e.is_valid()) {
    spdlog::error("Connector base tileset '{}' not found", base_tileset);
  } else {
    e.add<TilesetBase>(base_e);
  }

  if (!markings_tileset.empty()) {
    auto markings_e = world.lookup(("Textures::" + markings_tileset).c_str());
    if (!markings_e.is_valid()) {
      spdlog::error("Connector markings tileset '{}' not found",
                    markings_tileset);
    } else {
      e.add<TilesetMarkings>(markings_e);
    }
  }

  // A connector was added: flag the site so the autotiler recomputes every
  // tile's variant/rotation from its neighbours on the next relayout pass.
  site.add<SiteNeedsRelayout>();

  return e;
}

std::vector<flecs::entity> get_all_active_contracts(const flecs::world &world) {
  std::vector<flecs::entity> result;
  world.query_builder<Contract>().build().each([&](flecs::entity e,
                                                   Contract &) {
    if (e.has<ContractCurrentState>(world.lookup("States::Contract::Open")) ||
        e.has<ContractCurrentState>(
            world.lookup("States::Contract::Accepted"))) {
      result.push_back(e);
    }
  });
  return result;
}

// --- lua_CFunction wrappers ---

static int create_site_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  auto width = (uint32_t)luaL_checkinteger(L, 2);
  auto height = (uint32_t)luaL_checkinteger(L, 3);
  bool make_current = lua_toboolean(L, 4) != 0;
  lua_push_entity(
      L, create_site(*lua_get_world(L), name, width, height, make_current));
  return 1;
}

static int create_rocket_prefab_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_push_entity(L, create_rocket_prefab(*lua_get_world(L), name));
  return 1;
}

static int create_rocket_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *prefab = luaL_checkstring(L, 2);
  flecs::entity parent;
  if (!lua_isnoneornil(L, 3)) {
    parent = lua_check_entity(L, 3);
  }
  lua_push_entity(L, create_rocket(*lua_get_world(L), RocketName{name},
                                   RocketPrefabType{prefab}, parent));
  return 1;
}

static int create_building_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *prefab = luaL_checkstring(L, 2);
  auto x = (int)luaL_checkinteger(L, 3);
  auto y = (int)luaL_checkinteger(L, 4);
  flecs::entity site = lua_check_entity(L, 5);
  lua_push_entity(L,
                  create_building(*lua_get_world(L), name, prefab, x, y, site));
  return 1;
}

static int add_target_orbit_to_rocket_wrapper(lua_State *L) {
  flecs::entity rocket = lua_check_entity(L, 1);
  const char *orbit_name = luaL_checkstring(L, 2);
  auto max_mass = (uint32_t)luaL_checkinteger(L, 3);
  lua_push_entity(L, add_target_orbit_to_rocket(*lua_get_world(L), rocket,
                                                orbit_name, max_mass));
  return 1;
}

static int create_effect_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  flecs::entity source;
  if (!lua_isnoneornil(L, 2)) {
    source = lua_check_entity(L, 2);
  }
  lua_push_entity(L, create_effect(*lua_get_world(L), name, source));
  return 1;
}

static int create_contract_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *client = luaL_checkstring(L, 2);
  const char *description = luaL_checkstring(L, 3);
  auto upfront = (uint32_t)luaL_checknumber(L, 4);
  auto completion = (uint32_t)luaL_checknumber(L, 5);
  lua_push_entity(L, create_contract(*lua_get_world(L), name, client,
                                     description, upfront, completion));
  return 1;
}

static int create_contract_payload_wrapper(lua_State *L) {
  flecs::entity contract = lua_check_entity(L, 1);
  const char *name = luaL_checkstring(L, 2);
  auto mass = (uint32_t)luaL_checkinteger(L, 3);
  const char *target_orbit = luaL_optstring(L, 4, "");
  lua_push_entity(L, create_contract_payload(*lua_get_world(L), contract, name,
                                             mass, target_orbit));
  return 1;
}

static int get_all_contracts_wrapper(lua_State *L) {
  lua_newtable(L);
  int i = 1;
  for (auto e : get_all_contracts(*lua_get_world(L))) {
    lua_push_entity(L, e);
    lua_rawseti(L, -2, i++);
  }
  return 1;
}

static int get_all_active_contracts_wrapper(lua_State *L) {
  lua_newtable(L);
  int i = 1;
  for (auto e : get_all_active_contracts(*lua_get_world(L))) {
    lua_push_entity(L, e);
    lua_rawseti(L, -2, i++);
  }
  return 1;
}

static int add_modifier_wrapper(lua_State *L) {
  flecs::entity effect = lua_check_entity(L, 1);
  auto *ud =
      static_cast<ComponentUD *>(luaL_checkudata(L, 2, "solcorp.Modifier"));
  add_modifier(*lua_get_world(L), effect, *static_cast<Modifier *>(ud->ptr));
  return 0;
}

static int create_connector_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  auto variant = static_cast<ConnectorVariant>(luaL_checkinteger(L, 2));
  double rotation = luaL_checknumber(L, 3);
  auto x = static_cast<uint8_t>(luaL_checkinteger(L, 4));
  auto y = static_cast<uint8_t>(luaL_checkinteger(L, 5));
  flecs::entity site = lua_check_entity(L, 6);
  const char *base_tileset = luaL_checkstring(L, 7);
  const char *markings_tileset = luaL_optstring(L, 8, "");
  lua_push_entity(L,
                  create_connector(*lua_get_world(L), name, variant, rotation,
                                   x, y, site, base_tileset, markings_tileset));
  return 1;
}

void load_helpers_namespace(lua_State *L) {
  lua_getglobal(L, "solcorp");
  lua_get_or_create_table(L, "helpers");

  lua_register_function(L, "create_site", create_site_wrapper);
  lua_register_function(L, "create_building", create_building_wrapper);
  lua_register_function(L, "create_effect", create_effect_wrapper);
  lua_register_function(L, "create_rocket_prefab",
                        create_rocket_prefab_wrapper);
  lua_register_function(L, "create_rocket", create_rocket_wrapper);
  lua_register_function(L, "add_target_orbit_to_rocket",
                        add_target_orbit_to_rocket_wrapper);
  lua_register_function(L, "create_contract", create_contract_wrapper);
  lua_register_function(L, "create_contract_payload",
                        create_contract_payload_wrapper);
  lua_register_function(L, "get_all_contracts", get_all_contracts_wrapper);
  lua_register_function(L, "get_all_active_contracts",
                        get_all_active_contracts_wrapper);
  lua_register_function(L, "add_modifier", add_modifier_wrapper);
  lua_register_function(L, "create_connector", create_connector_wrapper);

  lua_pop(L, 2); // helpers, solcorp
}
