#include "lua.h"
#include "modules/lua/entity.h"
#include "modules/lua/helpers.h"
#include "modules/lua/logging.h"
#include "modules/lua/systems.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <flecs.h>

void load_mod_state(sol::state &mod_state);
void load_script_namespace(sol::state &mod_state);
void load_all_mods(flecs::world &world);
void load_mod(flecs::world &world, const std::filesystem::path &path);

LuaModule::LuaModule(flecs::world &world) {
  // Register components
  world.component<Mod>()
      .member<std::string>("name")  //
      .member<sol::state>("state"); //

  // Load mods
  load_all_mods(world);

  world.system<Mod>("Mod on_start Event")
      .kind(flecs::OnStart)
      .immediate()
      .each(mod_on_start);
}

void load_config_file() {
  sol::state config_state;
  config_state.open_libraries(sol::lib::base, sol::lib::package);
  sol::optional<sol::table> result =
      config_state.safe_script_file("config.lua", sol::script_pass_on_error);
  if (!result.has_value()) {
    spdlog::error("Failed to load config.lua");
    return;
  }
  sol::table config = result.value();
  spdlog::info("Font Name: {}",
               config.get_or("font", std::string{"DefaultFont.ttf"}));
  spdlog::info("Font Size: {}", config.get_or("font_size", 12));
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
                    entry.path().filename().c_str());
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

  mod.state["mod_name"] = mod_name;
  load_mod_state(mod.state);

  auto init_file = path / "init.lua";
  auto result =
      mod.state.safe_script_file(init_file.string(), sol::script_pass_on_error);
  if (!result.valid()) {
    sol::error err = result;
    spdlog::error("Failed to load {}: {}", init_file.string(), err.what());
    entity.destruct();
    return;
  }

  run_mod_handler(mod, world, "on_init");
}

/**
 * @brief Runs a callback on very mod
 *
 * @param world the flecs world
 * @param func The function to call
 */
void run_on_every_mod(flecs::world &world, const ModStateCallback &func) {
  auto mods = world.lookup("Mods");
  if (!mods.is_valid()) {
    spdlog::error("Mods entity does not exist!");
    return;
  }
  mods.children([&](flecs::entity modE) {
    auto mod = modE.get_mut<Mod>();
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
  mod.state["solcorp"]["world"] = &world;
  sol::protected_function function =
      mod.state["solcorp"]["script"]["handlers"][handler.c_str()];

  spdlog::info("Running mod handler {}", handler);
  auto result = function();
  if (!result.valid()) {
    sol::error err = result;
    spdlog::error("Could not run '{}' function: {}", handler, err.what());
    return false;
  }
  return true;
}

void load_mod_state(sol::state &mod_state) {
  mod_state.open_libraries(sol::lib::base, sol::lib::package);

  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();

  // Set up the state with internal functions
  load_logging(mod_state);
  load_script_namespace(mod_state);
  load_entity_usertype(mod_state);
  load_entities_namespace(mod_state);
  load_helpers_namespace(mod_state);
}

void load_script_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto script_ns = solcorp_ns["script"].get_or_create<sol::table>();
  auto handlers_ns = script_ns["handlers"].get_or_create<sol::table>();
}
