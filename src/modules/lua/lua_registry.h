#pragma once

#include <flecs.h>
#include <lua.hpp>
#include <string>

void lua_set_world(lua_State *L, flecs::world *world);
flecs::world *lua_get_world(lua_State *L);
void lua_set_mod_name(lua_State *L, const std::string &name);
std::string lua_get_mod_name(lua_State *L);
