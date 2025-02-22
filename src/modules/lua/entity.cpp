#include "entity.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void load_entities_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto entities = solcorp_ns["entities"].get_or_create<sol::table>();
  entities.set_function("create", [&mod_state](const std::string &name) {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    return world->entity(name.c_str());
  });
  entities.set_function("get", [&mod_state](const std::string &name) {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    flecs::entity e = world->lookup(name.c_str());
    return e;
  });

  entities.set_function("GameComponent", [&mod_state]() {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    return world->get_mut<Game>();
  });
}

void load_entity_usertype(sol::state &mod_state) {
  sol::usertype<flecs::entity> flecs_entity =
      mod_state.new_usertype<flecs::entity>("entity");
  flecs_entity["id"] = &flecs::entity::id;
  flecs_entity["destroy"] = &flecs::entity::destruct;
  flecs_entity["is_alive"] = &flecs::entity::is_alive;
  flecs_entity["name"] = &flecs::entity::name;
  flecs_entity["symbol"] = &flecs::entity::symbol;
  flecs_entity["enabled"] = [](flecs::entity &e) { return e.enabled(); };
  flecs_entity["enable"] = [](flecs::entity &e) { return e.enable(); };
  flecs_entity["disable"] = [](flecs::entity &e) { return e.disable(); };
  flecs_entity["child_of"] = [](flecs::entity &e, flecs::entity &rhs) {
    e.child_of(rhs);
  };
  flecs_entity["is_a"] = [](flecs::entity &e, const std::string &name) {
    auto world = e.world();
    std::string prefabPath = fmt::format("Prefabs::{}", name);
    auto prefab = world.lookup(prefabPath.c_str());
    if (!prefab.is_valid()) {
      spdlog::error("Cannot instantiate {} - Does not exist", prefabPath);
      return;
    }
    e.is_a(prefab);
  };
}
