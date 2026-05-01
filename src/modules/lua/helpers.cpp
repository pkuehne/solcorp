#include "helpers.h"
#include "modules/base/assert.h"
#include "modules/engine/render.h"
#include "modules/site/helpers.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <flecs.h>
#include <modules/rocket_launch/rocket_launch.h>
#include <sol/types.hpp>

flecs::world get_world(sol::this_state s) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
  return *world;
}

flecs::entity create_site_wrapper(sol::this_state s, const std::string &name,
                                  uint32_t width, uint32_t height,
                                  bool make_current) {
  return create_site(get_world(s), name, width, height, make_current);
}

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

flecs::entity create_building_prefab_wrapper(sol::this_state s,
                                             const std::string &name) {
  return create_building_prefab(get_world(s), name);
}

flecs::entity create_building_prefab(flecs::world world,
                                     const std::string &name) {
  auto buildings_node = world.lookup("Prefabs::Buildings");
  SC_ASSERT(buildings_node.is_valid(), "Prefabs::Buildings node not found");
  auto building_prefab = world.lookup("Prefabs::Core::Building");
  SC_ASSERT(building_prefab.is_valid(),
            "Prefabs::Core::Building prefab not found");
  return world.prefab(name.c_str()).is_a(building_prefab).child_of(buildings_node);
}

flecs::entity create_rocket_prefab_wrapper(sol::this_state s,
                                           const std::string &name) {
  return create_rocket_prefab(get_world(s), name);
}

flecs::entity create_rocket_prefab(flecs::world world,
                                   const std::string &name) {
  auto rockets_node = world.lookup("Prefabs::Rockets");
  SC_ASSERT(rockets_node.is_valid(), "Prefabs::Rockets node not found");
  auto rocket_prefab = world.lookup("Prefabs::Core::Rocket");
  SC_ASSERT(rocket_prefab.is_valid(), "Prefabs::Core::Rocket prefab not found");
  return world.prefab(name.c_str()).is_a(rocket_prefab).child_of(rockets_node);
}

flecs::entity add_facility_to_building_wrapper(sol::this_state s,
                                               flecs::entity building,
                                               const std::string &name) {
  return add_facility_to_building(get_world(s), building, name);
}

flecs::entity add_facility_to_building(flecs::world world,
                                       flecs::entity building,
                                       const std::string &name) {
  auto facility_prefab = world.lookup("Prefabs::Core::Facility");
  SC_ASSERT(facility_prefab.is_valid(),
            "Prefabs::Core::Facility prefab not found");
  return world.prefab(name.c_str()).is_a(facility_prefab).child_of(building);
}

flecs::entity create_rocket_wrapper(sol::this_state s, const std::string &name,
                                    const std::string &prefab,
                                    flecs::entity parent) {
  return create_rocket(get_world(s), name, prefab, parent);
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

flecs::entity create_building_wrapper(sol::this_state s,
                                      const std::string &name,
                                      const std::string &prefab, uint32_t x,
                                      uint32_t y, flecs::entity site) {
  return create_building(get_world(s), name, prefab, x, y, site);
}

flecs::entity create_building(flecs::world world, const std::string &name,
                              const std::string &prefab, uint32_t x,
                              uint32_t y, flecs::entity site) {
  return instantiateBuilding(world, name, prefab, x, y, site);
}

flecs::entity add_target_orbit_to_rocket_wrapper(sol::this_state s,
                                                  flecs::entity rocket,
                                                  const std::string &orbit_name,
                                                  uint32_t max_mass) {
  return add_target_orbit_to_rocket(get_world(s), rocket, orbit_name, max_mass);
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

flecs::entity create_texture_wrapper(sol::this_state s, const std::string &name,
                                      const std::string &filename) {
  sol::state_view mod_state(s);
  std::string mod_name = mod_state["solcorp"]["mod_name"]();
  return create_texture(get_world(s), name, filename, mod_name);
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

flecs::entity create_effect_wrapper(sol::this_state s, const std::string &name,
                                     flecs::entity source) {
  return create_effect(get_world(s), name, source);
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

flecs::entity add_modifier_wrapper(sol::this_state s, flecs::entity effect,
                                    Modifier mod) {
  return add_modifier(get_world(s), effect, mod);
}

flecs::entity add_modifier(flecs::world world, flecs::entity effect,
                           Modifier mod) {
  if (!effect.is_valid()) {
    return flecs::entity();
  }
  return world.entity().child_of(effect).set<Modifier>(mod);
}

Sprite clip_sprite_from_texture_wrapper(sol::this_state s,
                                         const std::string &texture,
                                         uint32_t x, uint32_t y,
                                         uint32_t width, uint32_t height) {
  return clip_sprite_from_texture(get_world(s), texture, x, y, width, height);
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

flecs::entity create_contract_wrapper(sol::this_state s,
                                       const std::string &name,
                                       const std::string &client,
                                       const std::string &description,
                                       float upfront_payment,
                                       float completion_payment) {
  return create_contract(get_world(s), name, client, description,
                         upfront_payment, completion_payment);
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

flecs::entity create_contract_payload_wrapper(sol::this_state s,
                                               flecs::entity contract,
                                               const std::string &name,
                                               uint32_t mass,
                                               const std::string &target_orbit_name) {
  return create_contract_payload(get_world(s), contract, name, mass,
                                 target_orbit_name);
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

sol::table get_all_contracts_wrapper(sol::this_state s) {
  sol::state_view mod_state(s);
  sol::table result = mod_state.create_table();
  int i = 1;
  for (auto e : get_all_contracts(get_world(s))) {
    result[i++] = e;
  }
  return result;
}

std::vector<flecs::entity> get_all_contracts(flecs::world world) {
  std::vector<flecs::entity> result;
  world.query_builder<Contract>().build().each(
      [&](flecs::entity e, Contract &) { result.push_back(e); });
  return result;
}

sol::table get_all_active_contracts_wrapper(sol::this_state s) {
  sol::state_view mod_state(s);
  sol::table result = mod_state.create_table();
  int i = 1;
  for (auto e : get_all_active_contracts(get_world(s))) {
    result[i++] = e;
  }
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

void load_helpers_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto helpers = solcorp_ns["helpers"].get_or_create<sol::table>();

  helpers.set_function("create_building_prefab", create_building_prefab_wrapper);
  helpers.set_function("create_site", create_site_wrapper);
  helpers.set_function("create_building", create_building_wrapper);
  helpers.set_function("add_facility_to_building", add_facility_to_building_wrapper);
  helpers.set_function("create_effect", create_effect_wrapper);
  helpers.set_function("create_texture", create_texture_wrapper);
  helpers.set_function("add_modifier", add_modifier_wrapper);
  helpers.set_function("clip_sprite_from_texture", clip_sprite_from_texture_wrapper);
  helpers.set_function("create_rocket_prefab", create_rocket_prefab_wrapper);
  helpers.set_function("create_rocket", create_rocket_wrapper);
  helpers.set_function("add_target_orbit_to_rocket", add_target_orbit_to_rocket_wrapper);
  helpers.set_function("create_contract", create_contract_wrapper);
  helpers.set_function("create_contract_payload", create_contract_payload_wrapper);
  helpers.set_function("get_all_contracts", get_all_contracts_wrapper);
  helpers.set_function("get_all_active_contracts", get_all_active_contracts_wrapper);
}
