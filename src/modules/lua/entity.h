#pragma once

#include <flecs.h>
#include <lua.hpp>

void load_entities_namespace(lua_State *L);
void load_entity_usertype(lua_State *L);

flecs::entity *lua_push_entity(lua_State *L, flecs::entity e);
flecs::entity lua_check_entity(lua_State *L, int idx);
