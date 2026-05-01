#include "entity.h"
#include "modules/lua/lua_registry.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void lua_push_entity(lua_State *L, flecs::entity e) {
  sol::stack::push(L, e);
}

flecs::entity lua_check_entity(lua_State *L, int idx) {
  return sol::stack::get<flecs::entity>(L, idx);
}

static int entities_create(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_push_entity(L, lua_get_world(L)->entity(name));
  return 1;
}

static int entities_get(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_push_entity(L, lua_get_world(L)->lookup(name));
  return 1;
}

void load_entities_namespace(lua_State *L) {
  lua_getglobal(L, "solcorp");
  lua_get_or_create_table(L, "entities");
  lua_register_function(L, "create", entities_create);
  lua_register_function(L, "get", entities_get);
  lua_pop(L, 2);

  // Game and Company return component pointers — sol3 until Stage 8
  sol::state_view sv(L);
  auto entities = sv["solcorp"]["entities"].get<sol::table>();
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
