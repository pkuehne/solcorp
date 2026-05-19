#include "lua.h"
#include "modules/base/base.h"
#include "modules/lua/entity.h"
#include "modules/lua/helpers.h"
#include "modules/lua/logging.h"
#include "modules/lua/lua_registry.h"
#include "modules/lua/systems.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <flecs.h>
#include <flecs/addons/cpp/world.hpp>

void load_mod_state(lua_State *L);
void load_script_namespace(lua_State *L);
void load_all_mods(flecs::world &world);
void load_mod(flecs::world &world, const std::filesystem::path &path);

LuaModule::LuaModule(flecs::world &world) {
  // Register components
  world.component<Mod>().member("name", &Mod::name); //
  //.member("state", &Mod::state); //

  // Load mods
  load_all_mods(world);

  world.system<Mod>("Mod on_start Event")
      .kind(PostStartPhase)
      .immediate()
      .each(mod_on_start);

  world.system<Mod>("Mod on_frame Event")
      .kind(flecs::OnUpdate)
      .each(mod_on_frame);

  auto sim = world.get<Simulation>();
  world.system<Mod>("Mod on_update Event")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(mod_on_update);
}

Config load_config_file() {
  Config config;

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  if (luaL_dofile(L, "config.lua") != LUA_OK) {
    spdlog::error("Failed to load config.lua: {}", lua_tostring(L, -1));
    lua_close(L);
    return config;
  }

  if (!lua_istable(L, -1)) {
    spdlog::error("config.lua must return a table");
    lua_close(L);
    return config;
  }

  lua_getfield(L, -1, "font");
  if (lua_isstring(L, -1)) {
    config.font = lua_tostring(L, -1);
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "font_size");
  if (lua_isnumber(L, -1)) {
    config.font_size = static_cast<uint32_t>(lua_tointeger(L, -1));
  }
  lua_pop(L, 1);

  spdlog::info("Config loaded: font='{}', font_size={}", config.font,
               config.font_size);

  lua_close(L);
  return config;
}

void load_all_mods(flecs::world &world) {
  auto scope = world.set_scope(0);
  world.entity("Mods");
  world.set_scope(scope);

  std::filesystem::path mod_path = "./mods/";
  if (!std::filesystem::exists(mod_path)) {
    std::filesystem::create_directory(mod_path);
  }

  for (const auto &entry : std::filesystem::directory_iterator(mod_path)) {
    if (!entry.is_directory()) {
      continue;
    }
    auto init_file = entry.path() / "init.lua";
    if (!std::filesystem::exists(init_file)) {
      spdlog::error("Mod in mods/{}/ does not have an init.lua file",
                    entry.path().filename().string());
      continue;
    }
    load_mod(world, entry.path());
  }
}

void load_mod(flecs::world &world, const std::filesystem::path &path) {
  auto mod_name = path.filename().string();
  spdlog::info("Loading mod {}", mod_name);
  auto mods = world.lookup("Mods");
  auto entity = world.entity(mod_name.c_str()).child_of(mods);
  auto &mod = entity.ensure<Mod>();
  mod.name = mod_name;
  mod.state = luaL_newstate();
  lua_set_mod_name(mod.state, mod_name);

  lua_newtable(mod.state);
  lua_setglobal(mod.state, "solcorp");

  load_mod_state(mod.state);

  auto init_file = path / "init.lua";
  if (luaL_loadfile(mod.state, init_file.string().c_str()) != LUA_OK ||
      lua_pcall(mod.state, 0, LUA_MULTRET, 0) != LUA_OK) {
    const char *err = lua_tostring(mod.state, -1);
    spdlog::error("Failed to load {}: {}", init_file.string(),
                  err ? err : "(unknown error)");
    lua_pop(mod.state, 1);
    entity.destruct();
    return;
  }

  run_mod_handler(mod, world, "on_init");
}

/**
 * @brief Runs a callback on every mod
 *
 * @param world the flecs world
 * @param func The function to call
 */
void run_on_every_mod(flecs::world &world, const ModStateCallback &func) {
  auto mods = world.lookup("Mods");
  if (!mods.is_valid()) {
    // spdlog::error("Mods entity does not exist!");
    return;
  }
  mods.children([&](flecs::entity modE) {
    Mod *mod = modE.try_get_mut<Mod>();
    if (!mod) {
      spdlog::error("Mod {} does not have a Mod component!",
                    modE.name().c_str());
      return;
    }
    func(mod->state);
  });
}

bool run_mod_handler(Mod &mod, flecs::world &world,
                     const std::string &handler) {
  lua_State *L = mod.state;
  lua_set_world(L, &world);

  lua_getglobal(L, "solcorp");
  lua_getfield(L, -1, "script");
  lua_getfield(L, -1, "handlers");
  lua_getfield(L, -1, handler.c_str());

  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 4);
    return true;
  }

  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    std::string err = lua_tostring(L, -1);
    lua_pop(L, 4); // errmsg, handlers, script, solcorp
    spdlog::error("{} - Could not run '{}' function: {}", mod.name, handler,
                  err);
    return false;
  }

  lua_pop(L, 3); // handlers, script, solcorp
  return true;
}

void load_mod_state(lua_State *L) {
  luaL_openlibs(L);

  // Set up the state with internal functions
  load_logging(L);
  load_script_namespace(L);
  load_entity_usertype(L);
  load_entities_namespace(L);
  load_helpers_namespace(L);
}

void register_enum_table_lua(
    flecs::world &world, const std::string &name,
    const std::function<void(LuaEnumBuilder &)> &register_func) {
  run_on_every_mod(world, [&name, &register_func](lua_State *L) {
    lua_newtable(L);
    int tbl_idx = lua_gettop(L);
    LuaEnumBuilder builder(L, tbl_idx);
    register_func(builder);
    lua_setglobal(L, name.c_str());
  });
}

void load_script_namespace(lua_State *L) {
  lua_getglobal(L, "solcorp");
  lua_get_or_create_table(L, "script");
  lua_get_or_create_table(L, "handlers");
  lua_pop(L, 3);
}
