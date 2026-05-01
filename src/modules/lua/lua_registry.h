#pragma once

#include <flecs.h>
#include <lua.hpp>
#include <string>

void lua_set_world(lua_State *L, flecs::world *world);
flecs::world *lua_get_world(lua_State *L);
void lua_set_mod_name(lua_State *L, const std::string &name);
std::string lua_get_mod_name(lua_State *L);

// Pushes L[-1][key], creating it as an empty table if absent. Net effect: +1.
void lua_get_or_create_table(lua_State *L, const char *key);

// Sets L[-1][name] = fn. Net effect: 0.
void lua_register_function(lua_State *L, const char *name, lua_CFunction fn);
