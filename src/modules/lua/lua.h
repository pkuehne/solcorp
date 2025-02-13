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
    sol::usertype<T> userType = state.new_usertype<T>(name);
    registerFunc(userType);

    std::string getter("get");
    getter.append(name);
    state["entity"][getter.c_str()] = [](flecs::entity &e) -> T * {
      auto component = &e.ensure<T>();
      return component;
    };
  });
}

struct LuaModule {
public:
  LuaModule(flecs::world &);
};
