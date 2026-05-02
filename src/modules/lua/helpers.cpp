#include "helpers.h"
#include "modules/base/assert.h"
#include "modules/engine/render.h"
#include "modules/lua/entity.h"
#include "modules/lua/lua_registry.h"
#include "modules/site/helpers.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <flecs.h>
#include <modules/rocket_launch/rocket_launch.h>

flecs::entity create_site(flecs::world world, const std::string &name,
                          uint32_t width, uint32_t height, bool make_current) {
  auto site = world.entity(name.c_str())
                  .set<Site>({width, height})
                  .set<Transform>({{340, 240}, {}})
                  .add<ConstructionSiteNeedsUpdating>();
  if (make_current) {
    site.add<CurrentSite>();
  }
  auto earth = world.lookup("Sun::Earth");
  SC_ASSERT(earth.is_valid(), "Sun::Earth entity not found");
  site.child_of(earth);
  return site;
}

flecs::entity create_building_prefab(flecs::world world,
                                     const std::string &name) {
  auto buildings_node = world.lookup("Prefabs::Buildings");
  SC_ASSERT(buildings_node.is_valid(), "Prefabs::Buildings node not found");
  auto building_prefab = world.lookup("Prefabs::Core::Building");
  SC_ASSERT(building_prefab.is_valid(),
            "Prefabs::Core::Building prefab not found");
  return world.prefab(name.c_str())
      .is_a(building_prefab)
      .child_of(buildings_node);
}

flecs::entity create_rocket_prefab(flecs::world world,
                                   const std::string &name) {
  auto rockets_node = world.lookup("Prefabs::Rockets");
  SC_ASSERT(rockets_node.is_valid(), "Prefabs::Rockets node not found");
  auto rocket_prefab = world.lookup("Prefabs::Core::Rocket");
  SC_ASSERT(rocket_prefab.is_valid(), "Prefabs::Core::Rocket prefab not found");
  return world.prefab(name.c_str()).is_a(rocket_prefab).child_of(rockets_node);
}

flecs::entity add_facility_to_building(flecs::world world,
                                       flecs::entity building,
                                       const std::string &name) {
  auto facility_prefab = world.lookup("Prefabs::Core::Facility");
  SC_ASSERT(facility_prefab.is_valid(),
            "Prefabs::Core::Facility prefab not found");
  return world.prefab(name.c_str()).is_a(facility_prefab).child_of(building);
}

flecs::entity create_rocket(flecs::world world, const std::string &name,
                            const std::string &prefab, flecs::entity parent) {
  std::string prefab_name = "Prefabs::Rockets::";
  prefab_name.append(prefab);
  auto prefab_entity = world.lookup(prefab_name.c_str());
  if (!prefab_entity.is_valid()) {
    spdlog::error("Rocket prefab {} does not exist", prefab_name);
    return flecs::entity();
  }
  auto rocket = world.entity(name.c_str()).is_a(prefab_entity);
  if (parent.is_valid()) {
    rocket.child_of(parent);
  }
  return rocket;
}

flecs::entity create_building(flecs::world world, const std::string &name,
                              const std::string &prefab, uint32_t x, uint32_t y,
                              flecs::entity site) {
  return instantiateBuilding(world, name, prefab, x, y, site);
}

flecs::entity add_target_orbit_to_rocket(flecs::world world,
                                         flecs::entity rocket,
                                         const std::string &orbit_name,
                                         uint32_t max_mass) {
  auto orbit = world.lookup(orbit_name.c_str());
  SC_ASSERT(orbit.is_valid(), fmt::format("Orbit {} not found", orbit_name));
  rocket.set<CanLiftTo>(orbit, {max_mass});
  return rocket;
}

flecs::entity create_texture(flecs::world world, const std::string &name,
                             const std::string &filename,
                             const std::string &mod_name) {
  if (filename.find("..") != std::string::npos) {
    spdlog::error("Invalid filename {}", filename);
    return flecs::entity();
  }
  auto location =
      (std::filesystem::path("mods") / mod_name / filename).string();
  auto texture_node = world.entity("Textures");
  return world.entity(name.c_str())
      .child_of(texture_node)
      .set<Texture>(loadTexture(location, world));
}

flecs::entity create_effect(flecs::world world, const std::string &name,
                            flecs::entity source) {
  auto effect = world.entity(name.c_str())
                    .add<Effect>()
                    .child_of(world.lookup("Effects"));
  if (source.is_valid()) {
    source.add<HasEffect>(effect);
  }
  return effect;
}

flecs::entity add_modifier(flecs::world world, flecs::entity effect,
                           Modifier mod) {
  if (!effect.is_valid()) {
    return flecs::entity();
  }
  return world.entity().child_of(effect).set<Modifier>(mod);
}

Sprite clip_sprite_from_texture(flecs::world world, const std::string &texture,
                                uint32_t x, uint32_t y, uint32_t width,
                                uint32_t height) {
  std::string texture_name("Textures::");
  texture_name.append(texture);
  auto textureE = world.lookup(texture_name.c_str());
  if (!textureE.is_valid()) {
    spdlog::error("Texture {} does not exist", texture);
    return Sprite();
  }
  Sprite sprite;
  sprite.texture = textureE;
  sprite.x = x;
  sprite.y = y;
  sprite.width = width;
  sprite.height = height;
  return sprite;
}

flecs::entity create_contract(flecs::world world, const std::string &name,
                              const std::string &client,
                              const std::string &description,
                              float upfront_payment, float completion_payment) {
  auto contracts_node = world.lookup("Contracts");
  SC_ASSERT(contracts_node.is_valid(), "Contracts node not found");
  auto existing = contracts_node.lookup(name.c_str());
  if (existing.is_valid()) {
    spdlog::warn("Entity with name {} already exists", name);
    return existing;
  }
  return world.entity(name.c_str())
      .set<Contract>({client, description, upfront_payment, completion_payment})
      .child_of(contracts_node);
}

flecs::entity create_contract_payload(flecs::world world,
                                      flecs::entity contract,
                                      const std::string &name, uint32_t mass,
                                      const std::string &target_orbit_name) {
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

std::vector<flecs::entity> get_all_contracts(flecs::world world) {
  std::vector<flecs::entity> result;
  world.query_builder<Contract>().build().each(
      [&](flecs::entity e, Contract &) { result.push_back(e); });
  return result;
}

std::vector<flecs::entity> get_all_active_contracts(flecs::world world) {
  std::vector<flecs::entity> result;
  world.query_builder<Contract>().build().each(
      [&](flecs::entity e, Contract &c) {
        if (c.status == ContractStatus::Open ||
            c.status == ContractStatus::Accepted) {
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

static int create_building_prefab_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_push_entity(L, create_building_prefab(*lua_get_world(L), name));
  return 1;
}

static int create_rocket_prefab_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_push_entity(L, create_rocket_prefab(*lua_get_world(L), name));
  return 1;
}

static int add_facility_to_building_wrapper(lua_State *L) {
  flecs::entity building = lua_check_entity(L, 1);
  const char *name = luaL_checkstring(L, 2);
  lua_push_entity(L,
                  add_facility_to_building(*lua_get_world(L), building, name));
  return 1;
}

static int create_rocket_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *prefab = luaL_checkstring(L, 2);
  flecs::entity parent;
  if (!lua_isnoneornil(L, 3)) {
    parent = lua_check_entity(L, 3);
  }
  lua_push_entity(L, create_rocket(*lua_get_world(L), name, prefab, parent));
  return 1;
}

static int create_building_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *prefab = luaL_checkstring(L, 2);
  auto x = (uint32_t)luaL_checkinteger(L, 3);
  auto y = (uint32_t)luaL_checkinteger(L, 4);
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

static int create_texture_wrapper(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *filename = luaL_checkstring(L, 2);
  lua_push_entity(L, create_texture(*lua_get_world(L), name, filename,
                                    lua_get_mod_name(L)));
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
  auto upfront = (float)luaL_checknumber(L, 4);
  auto completion = (float)luaL_checknumber(L, 5);
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

static int clip_sprite_from_texture_wrapper(lua_State *L) {
  const char *texture = luaL_checkstring(L, 1);
  auto x = (uint32_t)luaL_checkinteger(L, 2);
  auto y = (uint32_t)luaL_checkinteger(L, 3);
  auto width = (uint32_t)luaL_checkinteger(L, 4);
  auto height = (uint32_t)luaL_checkinteger(L, 5);
  auto *sprite = new Sprite(clip_sprite_from_texture(*lua_get_world(L), texture,
                                                     x, y, width, height));
  lua_push_component(L, sprite, "solcorp.Sprite", true,
                     [](void *p) { delete static_cast<Sprite *>(p); });
  return 1;
}

void load_helpers_namespace(lua_State *L) {
  lua_getglobal(L, "solcorp");
  lua_get_or_create_table(L, "helpers");

  lua_register_function(L, "create_building_prefab",
                        create_building_prefab_wrapper);
  lua_register_function(L, "create_site", create_site_wrapper);
  lua_register_function(L, "create_building", create_building_wrapper);
  lua_register_function(L, "add_facility_to_building",
                        add_facility_to_building_wrapper);
  lua_register_function(L, "create_effect", create_effect_wrapper);
  lua_register_function(L, "create_texture", create_texture_wrapper);
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
  lua_register_function(L, "clip_sprite_from_texture",
                        clip_sprite_from_texture_wrapper);

  lua_pop(L, 2); // helpers, solcorp
}
