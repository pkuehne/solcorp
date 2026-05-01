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
