#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <flecs.h>
#include <sol/sol.hpp>
#include <type_traits>

#include "modules/lua/entity.h"
#include "modules/lua/lua_registry.h"

// ─────────────────────────────────────────────────────────────────────────────

struct Mod {
  std::string name;
  sol::state state;
};

typedef const std::function<void(sol::state &)> ModStateCallback;

void load_config_file();
bool run_mod_handler(Mod &mod, flecs::world &world, const std::string &handler);
void run_on_every_mod(flecs::world &world, const ModStateCallback &func);

// ── Stage 8: raw Lua C API component registration ────────────────────────────

// Extracts class and field types from a member pointer type.
template <typename T>
struct member_ptr_info;
template <typename C, typename F>
struct member_ptr_info<F C::*> {
  using class_type = C;
  using field_type = F;
};

// Push a typed value onto the Lua stack.
template <typename F>
int lua_push_typed_value(lua_State *L, const F &val) {
  if constexpr (std::is_same_v<F, bool>) {
    lua_pushboolean(L, val ? 1 : 0);
  } else if constexpr (std::is_same_v<F, std::string>) {
    lua_pushstring(L, val.c_str());
  } else if constexpr (std::is_same_v<F, flecs::entity>) {
    lua_push_entity(L, val);
  } else if constexpr (std::is_integral_v<F>) {
    lua_pushinteger(L, static_cast<lua_Integer>(val));
  } else if constexpr (std::is_floating_point_v<F>) {
    lua_pushnumber(L, static_cast<lua_Number>(val));
  } else if constexpr (std::is_enum_v<F>) {
    lua_pushinteger(L, static_cast<lua_Integer>(val));
  } else {
    static_assert(sizeof(F) == 0, "Unsupported field type for lua_push_typed_value");
  }
  return 1;
}

// Read a typed value from the Lua stack at index idx.
template <typename F>
F lua_get_typed_value(lua_State *L, int idx) {
  if constexpr (std::is_same_v<F, bool>) {
    return lua_toboolean(L, idx) != 0;
  } else if constexpr (std::is_same_v<F, std::string>) {
    return std::string(luaL_checkstring(L, idx));
  } else if constexpr (std::is_same_v<F, flecs::entity>) {
    return lua_check_entity(L, idx);
  } else if constexpr (std::is_integral_v<F>) {
    return static_cast<F>(luaL_checkinteger(L, idx));
  } else if constexpr (std::is_floating_point_v<F>) {
    return static_cast<F>(luaL_checknumber(L, idx));
  } else if constexpr (std::is_enum_v<F>) {
    return static_cast<F>(luaL_checkinteger(L, idx));
  } else {
    static_assert(sizeof(F) == 0, "Unsupported field type for lua_get_typed_value");
  }
}

// Getter / setter lua_CFunctions templated on a member pointer.
// Non-capturing → can be used directly as lua_CFunction.
template <auto M>
static int field_getter(lua_State *L) {
  using C = typename member_ptr_info<decltype(M)>::class_type;
  using F = typename member_ptr_info<decltype(M)>::field_type;
  auto *ud = static_cast<ComponentUD *>(lua_touserdata(L, 1));
  return lua_push_typed_value<F>(L, static_cast<C *>(ud->ptr)->*M);
}

template <auto M>
static int field_setter(lua_State *L) {
  using C = typename member_ptr_info<decltype(M)>::class_type;
  using F = typename member_ptr_info<decltype(M)>::field_type;
  auto *ud = static_cast<ComponentUD *>(lua_touserdata(L, 1));
  static_cast<C *>(ud->ptr)->*M = lua_get_typed_value<F>(L, 2);
  return 0;
}

// Register a field via member pointer into a component metatable.
// Adds entries to the __getters and __setters sub-tables.
// mt_idx must be the absolute stack index of the metatable.
template <auto M>
void lua_register_field(lua_State *L, int mt_idx, const char *name) {
  lua_getfield(L, mt_idx, "__getters");
  lua_pushcfunction(L, field_getter<M>);
  lua_setfield(L, -2, name);
  lua_pop(L, 1);

  lua_getfield(L, mt_idx, "__setters");
  lua_pushcfunction(L, field_setter<M>);
  lua_setfield(L, -2, name);
  lua_pop(L, 1);
}

// Entity accessor templates — used as lua_CFunction via lua_pushcclosure.
// upvalue 1: metatable name string (for getT/setT).
template <typename T>
static int entity_getter(lua_State *L) {
  const char *mt = lua_tostring(L, lua_upvalueindex(1));
  flecs::entity e = lua_check_entity(L, 1);
  T *comp = &e.ensure<T>();
  lua_push_component(L, comp, mt, false, nullptr);
  return 1;
}

template <typename T>
static int entity_setter(lua_State *L) {
  const char *mt = lua_tostring(L, lua_upvalueindex(1));
  flecs::entity e = lua_check_entity(L, 1);
  auto *ud = static_cast<ComponentUD *>(luaL_checkudata(L, 2, mt));
  e.set<T>(*static_cast<T *>(ud->ptr));
  return 0;
}

template <typename T>
static int entity_haser(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  lua_pushboolean(L, e.has<T>() ? 1 : 0);
  return 1;
}

template <typename T>
static int entity_remover(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  e.remove<T>();
  return 0;
}

// Register a component type T with the Lua scripting system.
// Creates:
//   • metatable "solcorp.<name>" with __index/__newindex/__gc dispatch
//   • solcorp.components.<name> table with :new() constructor
//   • get<name>/set<name>/has<name>/remove<name> on the entity metatable
// register_fields(L, mt_idx) is called with the component metatable on the
// stack so that callers can add field accessors via lua_register_field.
template <typename T>
void register_component_lua(
    flecs::world &world, const char *name,
    const std::function<void(lua_State *, int)> &register_fields =
        [](lua_State *, int) {}) {

  run_on_every_mod(world, [name, &register_fields](sol::state &state) {
    lua_State *L = state.lua_state();
    std::string mt_name = std::string("solcorp.") + name;
    const char *mt = mt_name.c_str();

    // 1. Create the component metatable.
    luaL_newmetatable(L, mt);
    int mt_idx = lua_gettop(L);

    lua_newtable(L);
    lua_setfield(L, mt_idx, "__getters");
    lua_newtable(L);
    lua_setfield(L, mt_idx, "__setters");
    lua_pushcfunction(L, component_index);
    lua_setfield(L, mt_idx, "__index");
    lua_pushcfunction(L, component_newindex);
    lua_setfield(L, mt_idx, "__newindex");
    lua_pushcfunction(L, component_gc);
    lua_setfield(L, mt_idx, "__gc");

    register_fields(L, mt_idx);
    lua_pop(L, 1); // pop metatable

    // 2. Add solcorp.components.<name> with :new() constructor.
    lua_getglobal(L, "solcorp");
    lua_get_or_create_table(L, "components");
    lua_newtable(L); // class table
    lua_pushstring(L, mt);
    lua_pushcclosure(
        L,
        [](lua_State *Lx) -> int {
          const char *mtn = lua_tostring(Lx, lua_upvalueindex(1));
          auto *ud = static_cast<ComponentUD *>(
              lua_newuserdata(Lx, sizeof(ComponentUD)));
          ud->ptr = new T();
          ud->owned = true;
          ud->deleter = [](void *p) { delete static_cast<T *>(p); };
          luaL_getmetatable(Lx, mtn);
          lua_setmetatable(Lx, -2);
          return 1;
        },
        1);
    lua_setfield(L, -2, "new");
    lua_setfield(L, -2, name); // components[name] = class table
    lua_pop(L, 2);             // components, solcorp

    // 3. Add getT/setT/hasT/removeT to the entity metatable.
    luaL_getmetatable(L, "solcorp.entity");
    int e_mt = lua_gettop(L);

    lua_pushstring(L, mt);
    lua_pushcclosure(L, entity_getter<T>, 1);
    lua_setfield(L, e_mt, (std::string("get") + name).c_str());

    lua_pushstring(L, mt);
    lua_pushcclosure(L, entity_setter<T>, 1);
    lua_setfield(L, e_mt, (std::string("set") + name).c_str());

    lua_pushcfunction(L, entity_haser<T>);
    lua_setfield(L, e_mt, (std::string("has") + name).c_str());

    lua_pushcfunction(L, entity_remover<T>);
    lua_setfield(L, e_mt, (std::string("remove") + name).c_str());

    lua_pop(L, 1); // pop entity metatable
  });
}

// Register a global Lua table of integer enum values.
void register_enum_table_lua(
    flecs::world &world, const std::string &name,
    const std::function<void(lua_State *, int)> &register_func);

struct LuaModule {
public:
  LuaModule(flecs::world &);
};
