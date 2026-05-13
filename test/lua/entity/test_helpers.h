#pragma once
#include "modules/lua/entity.h"
#include "modules/lua/lua_registry.h"
#include <flecs.h>

inline lua_State *make_entity_state(flecs::world &world) {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  lua_set_world(L, &world);
  load_entity_usertype(L);
  return L;
}

inline void set_entity_global(lua_State *L, flecs::entity e) {
  lua_push_entity(L, e);
  lua_setglobal(L, "e");
}
