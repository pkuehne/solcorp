#define SOL_ALL_SAFETIES_ON 1
#include "entity.h"
#include "modules/lua/lua_registry.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

flecs::entity *lua_push_entity(lua_State *L, flecs::entity e) {
  auto *ud =
      static_cast<flecs::entity *>(lua_newuserdata(L, sizeof(flecs::entity)));
  *ud = e;
  luaL_getmetatable(L, "solcorp.entity");
  lua_setmetatable(L, -2);
  return ud;
}

flecs::entity lua_check_entity(lua_State *L, int idx) {
  return *static_cast<flecs::entity *>(
      luaL_checkudata(L, idx, "solcorp.entity"));
}

// ── entity methods ────────────────────────────────────────────────────────────

static int entity_id(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  lua_pushinteger(L, static_cast<lua_Integer>(e.id()));
  return 1;
}

static int entity_destroy(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  e.destruct();
  return 0;
}

static int entity_is_alive(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  lua_pushboolean(L, e.is_alive() ? 1 : 0);
  return 1;
}

static int entity_name(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  const char *n = e.name();
  lua_pushstring(L, n ? n : "");
  return 1;
}

static int entity_symbol(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  const char *s = e.symbol();
  lua_pushstring(L, s ? s : "");
  return 1;
}

static int entity_enabled(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  lua_pushboolean(L, e.enabled() ? 1 : 0);
  return 1;
}

static int entity_enable(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  e.enable();
  return 0;
}

static int entity_disable(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  e.disable();
  return 0;
}

static int entity_child_of(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  flecs::entity parent = lua_check_entity(L, 2);
  e.child_of(parent);
  return 0;
}

static int entity_lookup(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  const char *name = luaL_checkstring(L, 2);
  lua_push_entity(L, e.lookup(name));
  return 1;
}

static int entity_is_a(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  const char *name = luaL_checkstring(L, 2);
  auto world = e.world();
  std::string prefab_path = std::string("Prefabs::") + name;
  auto prefab = world.lookup(prefab_path.c_str());
  if (!prefab.is_valid()) {
    spdlog::error("Cannot instantiate {} - Does not exist", prefab_path);
    return 0;
  }
  e.is_a(prefab);
  return 0;
}

static const luaL_Reg entity_methods[] = {
    {"id", entity_id},           {"destroy", entity_destroy},
    {"is_alive", entity_is_alive}, {"name", entity_name},
    {"symbol", entity_symbol},   {"enabled", entity_enabled},
    {"enable", entity_enable},   {"disable", entity_disable},
    {"child_of", entity_child_of}, {"lookup", entity_lookup},
    {"is_a", entity_is_a},       {nullptr, nullptr}};

void load_entity_usertype(lua_State *L) {
  luaL_newmetatable(L, "solcorp.entity");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, entity_methods, 0);
  // Expose as global "entity" so register_lua_user_type<T> (Stage 8) can
  // add component accessors to this table without crashing on nil.
  lua_pushvalue(L, -1);
  lua_setglobal(L, "entity");
  lua_pop(L, 1);
}

// ── entities namespace ────────────────────────────────────────────────────────

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
