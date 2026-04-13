#include "helpers.h"
#include "modules/base/assert.h"
#include "modules/engine/render.h"
#include "modules/site/helpers.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include <flecs.h>
#include <modules/rocket_launch/rocket_launch.h>
#include <sol/types.hpp>

/// @brief Creates a new site entity in the ECS world.
///
/// Creates a site entity with the specified dimensions and optional current
/// site flag. The site is automatically set as a child of the Sun::Earth
/// entity. The site is initialized with Transform and
/// ConstructionSiteNeedsUpdating components.
///
/// @param s The Lua state handle (injected by sol2).
/// @param name The name identifier for the new site.
/// @param width The width of the site in grid units.
/// @param height The height of the site in grid units.
/// @param make_current If true, the site is marked as the current site.
/// @return A flecs::entity representing the newly created site.
/// @note The site is automatically positioned at coordinates (0, 50).
/// @note This function asserts that Sun::Earth entity exists in the world.
flecs::entity create_site(sol::this_state s, const std::string &name,
                          u_int width, u_int height,
                          bool make_current = false) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto site = world->entity(name.c_str())
                  .set<Site>({width, height})
                  .set<Transform>({{0, 50}, {}})
                  .add<ConstructionSiteNeedsUpdating>();
  if (make_current) {
    site.add<CurrentSite>();
  }
  auto earth = world->lookup("Sun::Earth");
  SC_ASSERT(earth.is_valid(), "Sun::Earth entity not found");
  site.child_of(earth);
  return site;
}

/// @brief Creates a new building prefab in the ECS world.
///
/// Creates a prefab entity that inherits from the core Building prefab and is
/// organized under the Prefabs::Buildings hierarchy. The prefab can be used as
/// a template to instantiate building entities with predefined components and
/// properties.
///
/// @param s The Lua state handle (injected by sol2).
/// @param name The name of the new building prefab to create.
/// @return A flecs::entity handle representing the newly created building
///         prefab.
/// @note This function asserts that both the Prefabs::Buildings node and
///       Prefabs::Core::Building prefab exist in the world. If either assertion
///       fails, the program will terminate.
flecs::entity create_building_prefab(sol::this_state s,
                                     const std::string &name) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto buildings_node = world->lookup("Prefabs::Buildings");
  SC_ASSERT(buildings_node.is_valid(), "Prefabs::Buildings node not found");
  auto building_prefab = world->lookup("Prefabs::Core::Building");
  SC_ASSERT(building_prefab.is_valid(),
            "Prefabs::Core::Building prefab not found");
  auto prefab = world->prefab(name.c_str())
                    .is_a(building_prefab)
                    .child_of(buildings_node);
  return prefab;
}

/// @brief Creates a new rocket prefab in the ECS world.
///
/// This function creates a prefab entity that inherits from the core Rocket
/// prefab and is organized under the Prefabs::Rockets hierarchy. The prefab can
/// be used as a template to instantiate rocket entities with predefined
/// components and properties.
///
/// @param s The Lua state handle (injected by sol2).
/// @param name The name of the new rocket prefab to create.
/// @return A flecs::entity handle representing the newly created rocket prefab.
/// @note This function asserts that both the Prefabs::Rockets node and
///       Prefabs::Core::Rocket prefab exist in the world. If either assertion
///       fails, the program will terminate.
flecs::entity create_rocket_prefab(sol::this_state s, const std::string &name) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto rockets_node = world->lookup("Prefabs::Rockets");
  SC_ASSERT(rockets_node.is_valid(), "Prefabs::Rockets node not found");
  auto rocket_prefab = world->lookup("Prefabs::Core::Rocket");
  SC_ASSERT(rocket_prefab.is_valid(), "Prefabs::Core::Rocket prefab not found");
  auto prefab =
      world->prefab(name.c_str()).is_a(rocket_prefab).child_of(rockets_node);
  return prefab;
}

/// @brief Adds a facility to a building entity.
///
/// This function creates a facility prefab entity as a child of the given
/// building entity. The facility inherits from the Prefabs::Core::Facility
/// prefab.
///
/// @param s The Lua state handle (injected by sol2).
/// @param building The parent building entity.
/// @param name The name of the new facility prefab.
/// @return A flecs::entity representing the newly created facility.
flecs::entity add_facility_to_building(sol::this_state s,
                                       flecs::entity building,
                                       const std::string &name) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto facility_prefab = world->lookup("Prefabs::Core::Facility");
  SC_ASSERT(facility_prefab.is_valid(),
            "Prefabs::Core::Facility prefab not found");
  auto prefab =
      world->prefab(name.c_str()).is_a(facility_prefab).child_of(building);
  return prefab;
}

/// @brief Creates a building entity in the ECS world through Lua bindings.
///
/// Instantiates a building from a prefab template within the given site.
/// This function is designed to be called from Lua code.
///
/// @param s The Lua state handle (injected by sol2).
/// @param name The name identifier for the building instance.
/// @param prefab The prefab template name to instantiate from.
/// @param x The X coordinate for building placement.
/// @param y The Y coordinate for building placement.
/// @param site The parent site entity to which this building belongs.
/// @return A flecs::entity representing the newly created building.
/// @note Retrieves the ECS world from the Lua state's solcorp module.
flecs::entity create_building(sol::this_state s, const std::string &name,
                              const std::string &prefab, u_int x, u_int y,
                              flecs::entity site) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  return instantiateBuilding(*world, name, prefab, x, y, site);
};

/// Adds a target orbit to a rocket entity and sets its maximum liftable mass.
///
/// This function is called from Lua and associates a rocket with a target
/// orbit, allowing it to lift cargo up to the specified maximum mass. The orbit
/// must exist in the ECS world; an assertion is raised if the orbit is not
/// found.
///
/// @param s he Lua state handle (injected by sol2).
/// @param rocket The ECS entity representing the rocket to configure.
/// @param orbit_name The name of the target orbit
/// @param max_mass The maximum mass (in kg) that the rocket can
/// lift.
/// @return The modified rocket entity with the CanLiftTo component set.
/// @throws Assertion failure if the orbit entity with the given name is not
/// found.
flecs::entity add_target_orbit_to_rocket(sol::this_state s,
                                         const flecs::entity rocket,
                                         const std::string &orbit_name,
                                         u_int max_mass) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto orbit = world->lookup(orbit_name.c_str());
  SC_ASSERT(orbit.is_valid(), fmt::format("Orbit {} not found", orbit_name));
  rocket.set<CanLiftTo>(orbit, {max_mass});

  return rocket;
};

/// @brief Creates a texture entity in the ECS world and loads it from a file.
///
/// Creates a new texture entity as a child of the "Textures" node. The texture
/// is loaded from a file path relative to the mod directory.
///
/// @param s The Lua state handle (injected by sol2).
/// @param name The name to assign to the texture entity.
/// @param filename The relative path to the texture file within the mod
///                 directory. Path traversal attempts (containing "..") are
///                 rejected for security.
/// @return A flecs::entity representing the created texture entity, or an empty
///         entity if the filename validation fails.
/// @note The file path is constructed as: "mods/{mod_name}/{filename}".
/// @note Returns an empty entity if filename contains "..".
flecs::entity create_texture(sol::this_state s, const std::string &name,
                             const std::string &filename) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  // validate filename
  if (filename.find("..") != std::string::npos) {
    spdlog::error("Invalid filename {}", filename);
    return flecs::entity();
  }
  std::string location = "mods/";
  location.append(mod_state["solcorp"]["mod_name"]());
  location.append("/");
  location.append(filename);
  auto texture_node = world->entity("Textures");
  auto texture = world->entity(name.c_str())
                     .child_of(texture_node)
                     .set<Texture>(loadTexture(location, *world));
  return texture;
};

/// @brief Creates a new effect entity in the ECS world and optionally
///        associates it with a source entity.
///
/// Creates an entity with the given name and adds the Effect component to it.
/// The entity is automatically added as a child of the "Effects" entity.
/// If a valid source entity is provided, it will be modified to include a
/// HasEffect component referencing the new effect.
///
/// @param s The Lua state handle (injected by sol2).
/// @param name The name of the effect entity to create.
/// @param source An optional source entity that will have the HasEffect
///               component added, referencing the created effect.
/// @return The newly created effect entity.
flecs::entity create_effect(sol::this_state s, const std::string &name,
                            flecs::entity source) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
  auto effect = world->entity(name.c_str())
                    .add<Effect>()
                    .child_of((world->lookup("Effects")));
  if (source.is_valid()) {
    source.add<HasEffect>(effect);
  }
  return effect;
};

/// @brief Adds a modifier as a child entity to the given effect entity.
///
/// Creates a new modifier entity as a child of the specified effect entity
/// and initializes it with the provided modifier data.
///
/// @param s The Lua state handle (injected by sol2).
/// @param effect The parent effect entity to which the modifier will be added.
///               Must be a valid entity.
/// @param mod The modifier data to be set on the newly created entity.
/// @return The newly created modifier entity as a child of the effect entity.
///         Returns an invalid entity if the provided effect entity is not
///         valid.
/// @note The modifier entity is automatically registered as a child of the
///       effect entity in the ECS hierarchy.
flecs::entity add_modifier(sol::this_state s, flecs::entity effect,
                           Modifier mod) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  if (!effect.is_valid()) {
    return flecs::entity();
  }
  auto modifier = world->entity().child_of(effect).set<Modifier>(mod);
  return modifier;
};

/// @brief Creates a sprite by clipping a region from an existing texture.
///
/// Retrieves a texture resource from the Flecs world and creates a sprite
/// with the specified clipping region (x, y, width, height).
///
/// @param s The Lua state handle (injected by sol2).
/// @param texture The name of the texture to clip from (without "Textures::"
///                prefix).
/// @param x The x-coordinate of the top-left corner of the clipping region.
/// @param y The y-coordinate of the top-left corner of the clipping region.
/// @param width The width of the clipping region in pixels.
/// @param height The height of the clipping region in pixels.
/// @return A Sprite object with the clipping region set. Returns an empty
///         Sprite object and logs an error if the texture is not found.
/// @note The texture name is automatically prefixed with "Textures::" when
///       looking it up in the world.
Sprite clip_sprite_from_texture(sol::this_state s, const std::string &texture,
                                u_int x, u_int y, u_int width, u_int height) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  std::string texture_name("Textures::");
  texture_name.append(texture);

  auto textureE = world->lookup(texture_name.c_str());
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

/// @brief Creates a Contract entity in the ECS world.
/// @param s
/// @param client The name of the client for the contract
/// @param description A brief description of the contract
/// @param upfront_payment The upfront payment amount
/// @param completion_payment The payment amount upon completion
/// @return
flecs::entity create_contract(sol::this_state s, const std::string &name,
                              const std::string &client,
                              const std::string &description,
                              float upfront_payment, float completion_payment) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto contracts_node = world->lookup("Contracts");
  SC_ASSERT(contracts_node.is_valid(), "Contracts node not found");

  // Check name doesn't already exist
  auto existing = contracts_node.lookup(name.c_str());
  if (existing.is_valid()) {
    spdlog::warn("Entity with name {} already exists", name);
    return existing;
  }

  auto contract_entity =
      world->entity(name.c_str())
          .set<Contract>(
              {client, description, upfront_payment, completion_payment})
          .child_of(contracts_node);
  return contract_entity;
}

/// @brief Creates a Payload entity and associates it with a Contract.
/// @param s
/// @param contract The contract to associate the payload with
/// @param name The name of the payload entity
/// @param mass  The mass of the payload in kg
/// @return The created Payload entity
flecs::entity create_contract_payload(sol::this_state s, flecs::entity contract,
                                      const std::string &name, u_int mass,
                                      const std::string &target_orbit_name) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto payload_entity =
      world->entity(name.c_str()).set<Payload>({mass}).child_of(contract);
  contract.add<ContractPayload>(payload_entity);
  if (!target_orbit_name.empty()) {
    auto orbit_entity = world->lookup(target_orbit_name.c_str());
    if (orbit_entity.is_valid()) {
      contract.add<ContractTargetOrbit>(orbit_entity);
    } else {
      spdlog::error("Orbit {} not found", target_orbit_name);
    }
  }
  return payload_entity;
}

/// @brief Retrieves all contracts in the ECS world.
///
/// Queries the ECS world for all entities with a Contract component and
/// returns them in a Lua table. The table keys are sequential integers
/// starting from 1.
///
/// @param s The Lua state handle (injected by sol2).
/// @return A Lua table containing all contract entities.
sol::table get_all_contracts(sol::this_state s) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  sol::table contracts_table = mod_state.create_table();

  int index = 1;
  world->query_builder<Contract>().build().each(
      [&](flecs::entity e, Contract &) { contracts_table[index++] = e; });

  return contracts_table;
}

/// @brief Registers helper functions into the Lua environment.
///
/// Sets up the solcorp.helpers namespace and binds C++ helper functions for
/// use in Lua scripts. The following functions are registered:
/// - create_building_prefab: Creates a building prefab definition.
/// - create_site: Creates a new site.
/// - create_building: Creates a building instance.
/// - add_facility_to_building: Adds a facility to an existing building.
/// - create_effect: Creates a visual or gameplay effect.
/// - create_texture: Creates a texture resource.
/// - add_modifier: Adds a modifier to an entity.
/// - clip_sprite_from_texture: Extracts a sprite region from a texture.
///
/// @param mod_state Reference to the sol2 Lua state where helpers will be
///                  registered.
/// @note Call during Lua environment initialization to make helper functions
///       available to Lua scripts.
void load_helpers_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto helpers = solcorp_ns["helpers"].get_or_create<sol::table>();

  helpers.set_function("create_building_prefab", create_building_prefab);
  helpers.set_function("create_site", create_site);
  helpers.set_function("create_building", create_building);
  helpers.set_function("add_facility_to_building", add_facility_to_building);
  helpers.set_function("create_effect", create_effect);
  helpers.set_function("create_texture", create_texture);
  helpers.set_function("add_modifier", add_modifier);
  helpers.set_function("clip_sprite_from_texture", clip_sprite_from_texture);
  helpers.set_function("create_rocket_prefab", create_rocket_prefab);
  helpers.set_function("add_target_orbit_to_rocket",
                       add_target_orbit_to_rocket);
  helpers.set_function("create_contract", create_contract);
  helpers.set_function("create_contract_payload", create_contract_payload);
  helpers.set_function("get_all_contracts", get_all_contracts);
}
