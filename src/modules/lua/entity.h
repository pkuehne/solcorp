#pragma once

#include <lua.hpp>
#include <sol/sol.hpp>

void load_entities_namespace(lua_State *L);
void load_entity_usertype(sol::state &mod_state);
