#include "lua_registry.h"
#include <stdexcept>

static constexpr const char *WORLD_KEY = "solcorp.world";
static constexpr const char *MOD_NAME_KEY = "solcorp.mod_name";

void lua_set_world(lua_State *L, flecs::world *world) {
  lua_pushlightuserdata(L, world);
  lua_setfield(L, LUA_REGISTRYINDEX, WORLD_KEY);
}

flecs::world *lua_get_world(lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, WORLD_KEY);
  auto *world = static_cast<flecs::world *>(lua_touserdata(L, -1));
  lua_pop(L, 1);
  return world;
}

void lua_set_mod_name(lua_State *L, const std::string &name) {
  lua_pushstring(L, name.c_str());
  lua_setfield(L, LUA_REGISTRYINDEX, MOD_NAME_KEY);
}

void lua_get_or_create_table(lua_State *L, const char *key) {
  lua_getfield(L, -1, key);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, key);
  }
}

void lua_register_function(lua_State *L, const char *name, lua_CFunction fn) {
  lua_pushcfunction(L, fn);
  lua_setfield(L, -2, name);
}

void lua_push_component(lua_State *L, void *ptr, const char *mt_name,
                        bool owned, void (*deleter)(void *)) {
  auto *ud =
      static_cast<ComponentUD *>(lua_newuserdata(L, sizeof(ComponentUD)));
  ud->ptr = ptr;
  ud->owned = owned;
  ud->deleter = deleter;
  luaL_getmetatable(L, mt_name);
  lua_setmetatable(L, -2);
}

int component_index(lua_State *L) {
  const char *key = lua_tostring(L, 2);
  if (!key) {
    lua_pushnil(L);
    return 1;
  }
  lua_getmetatable(L, 1);     // mt
  lua_getfield(L, -1, "__getters"); // mt, getters
  lua_getfield(L, -1, key);        // mt, getters, getters[key]
  if (!lua_isnil(L, -1)) {
    lua_pushvalue(L, 1);       // mt, getters, getter_fn, ud
    lua_call(L, 1, 1);         // mt, getters, result
    return 1;
  }
  lua_pop(L, 2); // pop nil + getters; mt remains
  lua_getfield(L, -1, key);   // mt[key] (method lookup)
  return 1;
}

int component_newindex(lua_State *L) {
  const char *key = lua_tostring(L, 2);
  if (!key)
    return luaL_error(L, "field name must be a string");
  lua_getmetatable(L, 1);
  lua_getfield(L, -1, "__setters");
  lua_getfield(L, -1, key);
  if (lua_isnil(L, -1))
    return luaL_error(L, "field '%s' is not settable", key);
  lua_pushvalue(L, 1); // ud
  lua_pushvalue(L, 3); // value
  lua_call(L, 2, 0);
  return 0;
}

int component_gc(lua_State *L) {
  auto *ud = static_cast<ComponentUD *>(lua_touserdata(L, 1));
  if (ud->owned && ud->deleter)
    ud->deleter(ud->ptr);
  return 0;
}

std::string lua_get_mod_name(lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, MOD_NAME_KEY);
  const char *name = lua_tostring(L, -1);
  std::string result = name ? name : "";
  lua_pop(L, 1);
  if (result.empty()) {
    throw std::runtime_error("Mod name not set.");
  }

  return result;
}
