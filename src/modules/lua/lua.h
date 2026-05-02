#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <flecs.h>
#include <sol/sol.hpp>

#include "modules/lua/entity.h"

// ── Sol3 compatibility bridge for the Stage 7 entity representation ───────────
//
// Stage 7 changed flecs::entity storage in Lua from sol3's pointer-indirection
// layout to a direct inline layout (lua_newuserdata(sizeof(flecs::entity)) with
// the entity value stored there, metatable "solcorp.entity").  These
// specialisations make every sol3 lambda that receives flecs::entity & work
// correctly with the new representation without requiring Stage 8 first.

namespace sol {

template <>
struct usertype_traits<flecs::entity> {
  static const std::string &name() {
    static const std::string n = "entity";
    return n;
  }
  static const std::string &qualified_name() {
    static const std::string q = "flecs::entity";
    return q;
  }
  // This is the key: sol3 checks THIS string against the Lua metatable.
  static const std::string &metatable() {
    static const std::string m = "solcorp.entity";
    return m;
  }
  static const std::string &user_metatable() {
    static const std::string u = "solcorp.entity.user";
    return u;
  }
  static const std::string &user_gc_metatable() {
    static const std::string u = "solcorp.entity.user\xE2\x99\xBB";
    return u;
  }
  static const std::string &gc_table() {
    static const std::string g = "solcorp.entity.\xE2\x99\xBB";
    return g;
  }
};

namespace stack {

// Push: use our layout (entity stored directly, not via pointer indirection).
template <>
struct unqualified_pusher<flecs::entity> {
  static int push(lua_State *L, const flecs::entity &e) {
    lua_push_entity(L, e);
    return 1;
  }
};

// Get: read the entity directly from the userdata block (no indirection).
// sol3's default getter does two-level indirection (void** → void* → T*),
// which does not match our layout.  Two specialisations are needed:
//
//  • unqualified_getter<flecs::entity>          — used when sol3 calls
//    stack::get<flecs::entity>(...) directly.
//  • unqualified_getter<detail::as_value_tag<flecs::entity>> — used for
//    flecs::entity& function parameters, because unqualified_getter<T&>
//    routes through as_value_tag<T> rather than unqualified_getter<T>.
template <>
struct unqualified_getter<flecs::entity> {
  static flecs::entity *get_no_lua_nil(lua_State *L, int index,
                                       record &tracking) {
    tracking.use(1);
    return static_cast<flecs::entity *>(lua_touserdata(L, index));
  }
  static flecs::entity &get(lua_State *L, int index, record &tracking) {
    return *get_no_lua_nil(L, index, tracking);
  }
};

template <>
struct unqualified_getter<detail::as_value_tag<flecs::entity>> {
  static flecs::entity *get_no_lua_nil(lua_State *L, int index,
                                       record &tracking) {
    tracking.use(1);
    return static_cast<flecs::entity *>(lua_touserdata(L, index));
  }
  static flecs::entity &get(lua_State *L, int index, record &tracking) {
    return *get_no_lua_nil(L, index, tracking);
  }
};

} // namespace stack
} // namespace sol

// ─────────────────────────────────────────────────────────────────────────────

struct Mod {
  std::string name;
  sol::state state;
};

typedef const std::function<void(sol::state &)> ModStateCallback;

void load_config_file();
bool run_mod_handler(Mod &mod, flecs::world &world, const std::string &handler);
void run_on_every_mod(flecs::world &world, const ModStateCallback &func);

template <typename T>
void register_lua_user_type(
    flecs::world &world, const std::string &name,
    const std::function<void(sol::usertype<T> &userType)> &registerFunc =
        [](sol::usertype<T> &) {}) {

  run_on_every_mod(world, [&name, &registerFunc](sol::state &state) {
    auto solcorp_ns = state["solcorp"].get_or_create<sol::table>();
    auto comp_ns = solcorp_ns["components"].get_or_create<sol::table>();
    sol::usertype<T> userType =
        comp_ns.new_usertype<T>(name, sol::constructors<T()>());
    registerFunc(userType);

    std::string getter("get");
    getter.append(name);
    state["entity"][getter.c_str()] = [](flecs::entity &e) -> T * {
      auto component = &e.ensure<T>();
      return component;
    };

    std::string setter("set");
    setter.append(name);
    state["entity"][setter.c_str()] = [](flecs::entity &e, T &c) -> void {
      e.set<T>(c);
    };

    std::string haver("has");
    haver.append(name);
    state["entity"][haver.c_str()] = [](flecs::entity &e) -> bool {
      return e.has<T>();
    };

    std::string remover("remove");
    remover.append(name);
    state["entity"][remover.c_str()] = [](flecs::entity &e) -> bool {
      return e.remove<T>();
    };
  });
}

template <typename T>
void register_lua_enum_table(
    flecs::world &world, const std::string &name,
    const std::function<void(sol::table &)> &registerFunc) {
  run_on_every_mod(world, [&name, &registerFunc](sol::state &state) {
    sol::table enum_table = state.create_table();
    registerFunc(enum_table);
    state[name] = enum_table;
  });
}

struct LuaModule {
public:
  LuaModule(flecs::world &);
};
