#include "entity.h"
#include "modules/lua/lua_registry.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void load_entities_namespace(lua_State *L) {
  sol::state_view sv(L);
  auto solcorp_ns = sv["solcorp"].get_or_create<sol::table>();
  auto entities = solcorp_ns["entities"].get_or_create<sol::table>();

  entities.set_function("create", [](sol::this_state s, const std::string &name) -> flecs::entity {
    lua_State *L = s;
    return lua_get_world(L)->entity(name.c_str());
  });
  entities.set_function("get", [](sol::this_state s, const std::string &name) -> flecs::entity {
    lua_State *L = s;
    return lua_get_world(L)->lookup(name.c_str());
  });
  entities.set_function("Game", [](sol::this_state s) -> Game * {
    lua_State *L = s;
    return &lua_get_world(L)->get_mut<Game>();
  });
  entities.set_function("Company", [](sol::this_state s) -> Company * {
    lua_State *L = s;
    return &lua_get_world(L)->get_mut<Company>();
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
  flecs_entity["lookup"] = [](flecs::entity &e, const std::string &name) {
    return e.lookup(name.c_str());
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
