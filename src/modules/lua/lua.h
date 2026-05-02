#pragma once

#include <flecs.h>
#include <functional>
#include <type_traits>

#include "modules/lua/entity.h"
#include "modules/lua/lua_registry.h"

// ─────────────────────────────────────────────────────────────────────────────

struct Mod {
  std::string name;
  lua_State *state;
};

typedef const std::function<void(lua_State *)> ModStateCallback;

void load_config_file();
bool run_mod_handler(Mod &mod, flecs::world &world, const std::string &handler);
void run_on_every_mod(flecs::world &world, const ModStateCallback &func);

// ── Stage 8: component / enum registration helpers ───────────────────────────

// Extracts class and field types from a member pointer type.
template <typename T> struct member_ptr_info;
template <typename C, typename F> struct member_ptr_info<F C::*> {
  using class_type = C;
  using field_type = F;
};

// Push a typed value onto the Lua stack.
template <typename F> int lua_push_typed_value(lua_State *L, const F &val) {
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
    static_assert(sizeof(F) == 0,
                  "Unsupported field type for lua_push_typed_value");
  }
  return 1;
}

// Read a typed value from the Lua stack at index idx.
template <typename F> F lua_get_typed_value(lua_State *L, int idx) {
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
    static_assert(sizeof(F) == 0,
                  "Unsupported field type for lua_get_typed_value");
  }
}

// Getter / setter lua_CFunctions templated on a member pointer.
// Non-capturing → can be used directly as lua_CFunction.
template <auto M> static int field_getter(lua_State *L) {
  using C = typename member_ptr_info<decltype(M)>::class_type;
  using F = typename member_ptr_info<decltype(M)>::field_type;
  auto *ud = static_cast<ComponentUD *>(lua_touserdata(L, 1));
  return lua_push_typed_value<F>(L, static_cast<C *>(ud->ptr)->*M);
}

template <auto M> static int field_setter(lua_State *L) {
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
template <typename T> static int entity_getter(lua_State *L) {
  const char *mt = lua_tostring(L, lua_upvalueindex(1));
  flecs::entity e = lua_check_entity(L, 1);
  T *comp = &e.ensure<T>();
  lua_push_component(L, comp, mt, false, nullptr);
  return 1;
}

template <typename T> static int entity_setter(lua_State *L) {
  const char *mt = lua_tostring(L, lua_upvalueindex(1));
  flecs::entity e = lua_check_entity(L, 1);
  auto *ud = static_cast<ComponentUD *>(luaL_checkudata(L, 2, mt));
  e.set<T>(*static_cast<T *>(ud->ptr));
  return 0;
}

template <typename T> static int entity_haser(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  lua_pushboolean(L, e.has<T>() ? 1 : 0);
  return 1;
}

template <typename T> static int entity_remover(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  e.remove<T>();
  return 0;
}

// ── Builder helpers for register_component_lua and register_enum_table_lua ───

// Fluent builder passed to the register_component_lua callback.
// Hides the raw Lua C API from callers.
template <typename C> class LuaFieldBuilder {
public:
  LuaFieldBuilder(lua_State *L, int mt_idx) : L_(L), mt_(mt_idx) {}

  // Register a plain struct member field (read + write).
  template <auto M> LuaFieldBuilder &field(const char *name) {
    lua_register_field<M>(L_, mt_, name);
    return *this;
  }

  // Register a getter-only field that returns a nested component (e.g. a Stat
  // member). The component_type is the unqualified type name; "solcorp." is
  // prepended automatically.
  template <auto M>
  LuaFieldBuilder &nested(const char *name, const char *component_type) {
    using CT = typename member_ptr_info<decltype(M)>::class_type;
    lua_getfield(L_, mt_, "__getters");
    lua_pushstring(L_, (std::string("solcorp.") + component_type).c_str());
    lua_pushcclosure(
        L_,
        [](lua_State *Lx) -> int {
          const char *mt = lua_tostring(Lx, lua_upvalueindex(1));
          auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
          lua_push_component(Lx, &(static_cast<CT *>(ud->ptr)->*M), mt, false,
                             nullptr);
          return 1;
        },
        1);
    lua_setfield(L_, -2, name);
    lua_pop(L_, 1);
    return *this;
  }

  // Register a computed getter-only field via a constexpr callable
  // (non-capturing lambda). The return type is deduced automatically.
  //   b.getter<[](const MyComp* c) { return c->method(); }>("fieldName")
  template <auto Getter> LuaFieldBuilder &getter(const char *name) {
    lua_getfield(L_, mt_, "__getters");
    lua_pushcfunction(L_, [](lua_State *Lx) -> int {
      using R =
          std::remove_cvref_t<decltype(Getter(std::declval<const C *>()))>;
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      return lua_push_typed_value<R>(Lx,
                                     Getter(static_cast<const C *>(ud->ptr)));
    });
    lua_setfield(L_, -2, name);
    lua_pop(L_, 1);
    return *this;
  }

  // Register a computed field with both getter and setter via constexpr
  // callables (non-capturing lambdas). The value type is deduced from Getter.
  //   b.computed<
  //       [](const MyComp* c) { return c->value(); },
  //       [](MyComp* c, double v) { c->setValue(v); }
  //   >("fieldName")
  template <auto Getter, auto Setter>
  LuaFieldBuilder &computed(const char *name) {
    using R = std::remove_cvref_t<decltype(Getter(std::declval<const C *>()))>;

    lua_getfield(L_, mt_, "__getters");
    lua_pushcfunction(L_, [](lua_State *Lx) -> int {
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      return lua_push_typed_value<R>(Lx,
                                     Getter(static_cast<const C *>(ud->ptr)));
    });
    lua_setfield(L_, -2, name);
    lua_pop(L_, 1);

    lua_getfield(L_, mt_, "__setters");
    lua_pushcfunction(L_, [](lua_State *Lx) -> int {
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      R val = lua_get_typed_value<R>(Lx, 2);
      Setter(static_cast<C *>(ud->ptr), val);
      return 0;
    });
    lua_setfield(L_, -2, name);
    lua_pop(L_, 1);

    return *this;
  }

private:
  lua_State *L_;
  int mt_;
};

// Fluent builder passed to the register_enum_table_lua callback.
class LuaEnumBuilder {
public:
  LuaEnumBuilder(lua_State *L, int tbl_idx) : L_(L), tbl_(tbl_idx) {}

  // Add an integer enum value to the Lua table.
  template <typename E> LuaEnumBuilder &value(const char *name, E val) {
    lua_pushinteger(L_, static_cast<lua_Integer>(val));
    lua_setfield(L_, tbl_, name);
    return *this;
  }

private:
  lua_State *L_;
  int tbl_;
};

// ── register_component_lua / register_enum_table_lua ─────────────────────────

// Register a component type T with the Lua scripting system.
// Creates:
//   • metatable "solcorp.<name>" with __index/__newindex/__gc dispatch
//   • solcorp.components.<name> table with :new() constructor
//   • get<name>/set<name>/has<name>/remove<name> on the entity metatable
// register_fields receives a LuaFieldBuilder<T> so callers can register
// fields without touching the raw Lua C API.
template <typename T>
void register_component_lua(
    flecs::world &world, const char *name,
    const std::function<void(LuaFieldBuilder<T> &)> &register_fields =
        [](LuaFieldBuilder<T> &) {}) {

  run_on_every_mod(world, [name, &register_fields](lua_State *L) {
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

    LuaFieldBuilder<T> builder(L, mt_idx);
    register_fields(builder);
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
// The callback receives a LuaEnumBuilder to add entries without raw Lua API.
void register_enum_table_lua(
    flecs::world &world, const std::string &name,
    const std::function<void(LuaEnumBuilder &)> &register_func =
        [](LuaEnumBuilder &) {});

struct LuaModule {
public:
  LuaModule(flecs::world &);
};
