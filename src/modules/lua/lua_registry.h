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

// Wrapper for a component pointer stored in a Lua full userdata.
// getT() produces owned=false (ECS memory, no gc); T:new() produces owned=true
// (heap memory, freed on gc).  All field accessors receive a ComponentUD*.
struct ComponentUD {
  void *ptr;
  bool owned;
  void (*deleter)(void *); // called if owned=true; nullptr for ECS-backed
};

// Push a ComponentUD userdata onto the Lua stack.
void lua_push_component(lua_State *L, void *ptr, const char *mt_name,
                        bool owned, void (*deleter)(void *));

// Generic __index, __newindex, __gc for ComponentUD metatables.
// Look up __getters / __setters sub-tables; fall back to the metatable for
// method lookups (__index only).
int component_index(lua_State *L);
int component_newindex(lua_State *L);
int component_gc(lua_State *L);
