/**
 * @file lua.h
 * @brief Lua integration helpers for component and enum registration.
 *
 * @details
 * This header defines the registration surface used by modules when exposing
 * ECS components to Lua through register_component_lua().
 *
 * Property helper selection guide:
 * - field<M>("name")
 *   Use for direct struct members that can be copied as values
 *   (numbers, bool, string, enums, flecs::entity).
 *   Example: field<&Game::day>("day")
 *
 * - nested<M>("name", "Type")
 *   Use when the member is another component-like object that should be
 *   exposed as userdata, not copied by value.
 *   Example: nested<&Rocket::cost>({"cost"}, {"Stat"})
 *
 * - getter<G>("name")
 *   Read-only property backed by a method or derived value.
 *   G is a non-capturing lambda that takes const Component* and returns the
 *   value to expose.
 *   Example: getter<[](const Stat* s) { return s->value(); }>("value")
 *
 * - computed<G, S>("name")
 *   Read/write property backed by methods instead of a direct member.
 *   G (getter) is a non-capturing lambda: const Component* -> Value.
 *   S (setter) is a non-capturing lambda: Component*, Value -> void.
 *   Example:
 *   computed<
 *     [](const Stat* s) { return s->base(); },
 *     [](Stat* s, double v) { s->setBase(v); }
 *   >("base")
 */
#pragma once

#include <concepts>
#include <flecs.h>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include "modules/lua/entity.h"
#include "modules/lua/lua_registry.h"

/**
 * @brief Loaded Lua mod state attached to an ECS entity under Mods.
 */
struct Mod {
  std::string name;
  lua_State *state;
};

using ModStateCallback = std::function<void(lua_State *)>;

/** @brief Load config.lua and apply global startup settings. */
void load_config_file();

/**
 * @brief Execute a named script handler on a specific mod.
 * @return true if the handler either does not exist or runs successfully.
 */
bool run_mod_handler(Mod &mod, flecs::world &world, const std::string &handler);

/**
 * @brief Run a callback for each loaded mod state.
 */
void run_on_every_mod(flecs::world &world, const ModStateCallback &func);

/** @brief Extract class/field types from a member pointer type. */
template <typename T> struct member_ptr_info;
template <typename C, typename F> struct member_ptr_info<F C::*> {
  using class_type = C;
  using field_type = F;
};

/** @brief Types that can be marshalled by
 * lua_push_typed_value/lua_get_typed_value. */
template <typename F>
concept LuaValueType = std::is_same_v<std::remove_cvref_t<F>, bool> ||
                       std::is_same_v<std::remove_cvref_t<F>, std::string> ||
                       std::is_same_v<std::remove_cvref_t<F>, flecs::entity> ||
                       std::is_integral_v<std::remove_cvref_t<F>> ||
                       std::is_floating_point_v<std::remove_cvref_t<F>> ||
                       std::is_enum_v<std::remove_cvref_t<F>>;

/** @brief Valid direct field pointer for value marshalling. */
template <auto M>
concept LuaFieldMemberPointer =
    std::is_member_object_pointer_v<decltype(M)> && requires {
      typename member_ptr_info<decltype(M)>::field_type;
    } && LuaValueType<typename member_ptr_info<decltype(M)>::field_type>;

/** @brief Valid direct field pointer for a specific component type. */
template <auto M, typename C>
concept LuaFieldMemberPointerOf =
    LuaFieldMemberPointer<M> &&
    std::same_as<typename member_ptr_info<decltype(M)>::class_type, C>;

/** @brief Valid member-object pointer for a specific component type. */
template <auto M, typename C>
concept MemberObjectPointerOf =
    std::is_member_object_pointer_v<decltype(M)> &&
    std::same_as<typename member_ptr_info<decltype(M)>::class_type, C>;

/** @brief Valid getter callable for component type C. */
template <auto Getter, typename C>
concept LuaGetterFor =
    requires(const C *c) { Getter(c); } &&
    LuaValueType<
        std::remove_cvref_t<decltype(Getter(std::declval<const C *>()))>>;

/** @brief Valid getter/setter callable pair for component type C. */
template <auto Getter, auto Setter, typename C>
concept LuaComputedFor =
    LuaGetterFor<Getter, C> &&
    requires(
        C *c,
        std::remove_cvref_t<decltype(Getter(std::declval<const C *>()))> v) {
      { Setter(c, v) } -> std::same_as<void>;
    };

/** @brief Push a supported C++ value type onto the Lua stack. */
template <LuaValueType F> int lua_push_typed_value(lua_State *L, const F &val) {
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
  }
  return 1;
}

/** @brief Read a supported C++ value type from Lua stack index idx. */
template <LuaValueType F> F lua_get_typed_value(lua_State *L, int idx) {
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
  }
}

/**
 * @brief Lua getter shim for a direct member pointer.
 */
template <auto M>
  requires LuaFieldMemberPointer<M>
static int field_getter(lua_State *L) {
  using C = typename member_ptr_info<decltype(M)>::class_type;
  using F = typename member_ptr_info<decltype(M)>::field_type;
  auto *ud = static_cast<ComponentUD *>(lua_touserdata(L, 1));
  return lua_push_typed_value<F>(L, static_cast<C *>(ud->ptr)->*M);
}

template <auto M>
  requires LuaFieldMemberPointer<M>
static int field_setter(lua_State *L) {
  using C = typename member_ptr_info<decltype(M)>::class_type;
  using F = typename member_ptr_info<decltype(M)>::field_type;
  auto *ud = static_cast<ComponentUD *>(lua_touserdata(L, 1));
  static_cast<C *>(ud->ptr)->*M = lua_get_typed_value<F>(L, 2);
  return 0;
}

/**
 * @brief Register a direct member field in __getters and __setters.
 * @param L Lua state.
 * @param mt_idx Absolute stack index of the component metatable.
 * @param name Lua field name.
 */
template <auto M>
  requires LuaFieldMemberPointer<M>
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

/** @brief Entity get<T>() bridge. */
template <typename T> static int entity_getter(lua_State *L) {
  const char *mt = lua_tostring(L, lua_upvalueindex(1));
  flecs::entity e = lua_check_entity(L, 1);
  T *comp = &e.ensure<T>();
  lua_push_component(L, comp, mt, false, nullptr);
  return 1;
}

/** @brief Entity set<T>() bridge. */
template <typename T> static int entity_setter(lua_State *L) {
  const char *mt = lua_tostring(L, lua_upvalueindex(1));
  flecs::entity e = lua_check_entity(L, 1);
  auto *ud = static_cast<ComponentUD *>(luaL_checkudata(L, 2, mt));
  e.set<T>(*static_cast<T *>(ud->ptr));
  return 0;
}

/** @brief Entity has<T>() bridge. */
template <typename T> static int entity_haser(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  lua_pushboolean(L, e.has<T>() ? 1 : 0);
  return 1;
}

/** @brief Entity remove<T>() bridge. */
template <typename T> static int entity_remover(lua_State *L) {
  flecs::entity e = lua_check_entity(L, 1);
  e.remove<T>();
  return 0;
}

struct LuaFieldName {
  const char *value;
};
struct LuaComponentType {
  const char *value;
};

/**
 * @brief Fluent component field registration helper.
 *
 * Passed to register_component_lua() callbacks so modules can expose
 * properties without directly touching Lua stack manipulation.
 */
template <typename C> class LuaFieldBuilder {
public:
  LuaFieldBuilder(lua_State *L, int mt_idx) : L_(L), mt_(mt_idx) {}

  /**
   * @brief Register a direct member field (read and write).
   * @details Use for plain value-like members such as numbers, booleans,
   * strings, enums, and flecs::entity.
   */
  template <auto M>
    requires LuaFieldMemberPointerOf<M, C>
  LuaFieldBuilder &field(const char *name) {
    lua_register_field<M>(L_, mt_, name);
    return *this;
  }

  /**
   * @brief Register a getter-only nested component field.
   * @details Use when a member should be exposed as component userdata
   * (for example a nested Stat), not copied as a plain value.
   * @param name Lua field name.
   * @param component_type Unqualified component type name ("solcorp." is
   * prepended automatically).
   */
  template <auto M>
    requires MemberObjectPointerOf<M, C>
  LuaFieldBuilder &nested(LuaFieldName name, LuaComponentType component_type) {
    using CT = typename member_ptr_info<decltype(M)>::class_type;
    lua_getfield(L_, mt_, "__getters");
    lua_pushstring(L_,
                   (std::string("solcorp.") + component_type.value).c_str());
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
    lua_setfield(L_, -2, name.value);
    lua_pop(L_, 1);
    return *this;
  }

  /**
   * @brief Register a computed read-only property.
   * @details Use when data is exposed through methods or derived logic rather
   * than a direct member.
   */
  template <auto Getter>
    requires LuaGetterFor<Getter, C>
  LuaFieldBuilder &getter(const char *name) {
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

  /**
   * @brief Register a computed read/write property.
   * @details Use when Lua should read/write via explicit methods instead of a
   * direct member pointer (for example base()/setBase()).
   */
  template <auto Getter, auto Setter>
    requires LuaComputedFor<Getter, Setter, C>
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

/**
 * @brief Fluent enum table registration helper.
 */
class LuaEnumBuilder {
public:
  LuaEnumBuilder(lua_State *L, int tbl_idx) : L_(L), tbl_(tbl_idx) {}

  /** @brief Add a named enum constant to the Lua table. */
  template <typename E> LuaEnumBuilder &value(const char *name, E val) {
    lua_pushinteger(L_, static_cast<lua_Integer>(val));
    lua_setfield(L_, tbl_, name);
    return *this;
  }

private:
  lua_State *L_;
  int tbl_;
};

/**
 * @brief Register a component type with Lua accessors and constructors.
 *
 * @details
 * This function creates:
 * - metatable "solcorp.<name>" with __index/__newindex/__gc dispatch,
 * - solcorp.components.<name> with :new() constructor,
 * - entity methods get<name>/set<name>/has<name>/remove<name>.
 *
 * @param world Flecs world.
 * @param name Component name suffix used for Lua symbols.
 * @param register_fields Callback that receives LuaFieldBuilder<T> to define
 * exposed properties.
 */
template <typename T>
  requires std::default_initializable<T>
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

/**
 * @brief Register a global Lua enum-like table.
 * @param world Flecs world.
 * @param name Global Lua table name.
 * @param register_func Callback used to add values through LuaEnumBuilder.
 */
void register_enum_table_lua(
    flecs::world &world, const std::string &name,
    const std::function<void(LuaEnumBuilder &)> &register_func =
        [](LuaEnumBuilder &) {});

struct LuaModule {
public:
  LuaModule(flecs::world &);
};
