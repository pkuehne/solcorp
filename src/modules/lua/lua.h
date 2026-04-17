#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <flecs.h>
#include <sol/sol.hpp>

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
