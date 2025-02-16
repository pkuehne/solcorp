#include "lua.h"
#include "flecs/addons/cpp/entity.hpp"
#include "modules/engine/render.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <memory>

void load_mod_state(sol::state &mod_state);
void load_entities_namespace(sol::state &mod_state);
void load_script_namespace(sol::state &mod_state);
void load_helpers_namespace(sol::state &mod_state);
void load_entity_usertype(sol::state &mod_state);
void load_logging(sol::state &mod_state);
void load_mod(flecs::world &world, const std::filesystem::path &path);

LuaModule::LuaModule(flecs::world &world) {
  // Register components
  world.component<Mod>()
      .member<std::string>("name")  //
      .member<sol::state>("state"); //

  // Load mods
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

void load_config_file() {
  sol::state config_state;
  config_state.open_libraries(sol::lib::base, sol::lib::package);
  sol::optional<sol::table> result =
      config_state.safe_script_file("config.lua", sol::script_pass_on_error);
  if (!result.has_value()) {
    spdlog::error("Failed to load config.lua");
  } else {
    sol::table config = result.value();
    spdlog::info("Font Name: {}",
                 config.get_or("font", std::string{"DefaultFont.ttf"}));
    spdlog::info("Font Size: {}", config.get_or("font_size", 12));
    config_state.script("print('go go go!')");
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

void load_logging(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto logging_ns = solcorp_ns["logging"].get_or_create<sol::table>();

  std::string mod_name = mod_state["mod_name"];
  auto sink = spdlog::default_logger()->sinks()[0];
  auto logger = std::make_shared<spdlog::logger>(mod_name, sink);
  spdlog::register_logger(logger);

  logging_ns.set_function("info", [&mod_state](const std::string &message) {
    std::string mod_name = mod_state["mod_name"];
    auto logger = spdlog::get(mod_name);
    if (logger) {
      logger->info("{}", message);
    }
  });
  logging_ns.set_function("warn", [&mod_state](const std::string &message) {
    std::string mod_name = mod_state["mod_name"];
    auto logger = spdlog::get(mod_name);
    if (logger) {
      logger->warn("{}", message);
    }
  });
  logging_ns.set_function("error", [&mod_state](const std::string &message) {
    std::string mod_name = mod_state["mod_name"];
    auto logger = spdlog::get(mod_name);
    if (logger) {
      logger->error("{}", message);
    }
  });
}

void load_entities_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto entities = solcorp_ns["entities"].get_or_create<sol::table>();
  entities.set_function("create", [&mod_state](const std::string &name) {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    return world->entity(name.c_str());
  });
  entities.set_function("get", [&mod_state](const std::string &name) {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    flecs::entity e = world->lookup(name.c_str());
    return e;
  });

  entities.set_function("GameComponent", [&mod_state]() {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    return world->get_mut<Game>();
  });
}

void load_helpers_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto helpers = solcorp_ns["helpers"].get_or_create<sol::table>();

  helpers.set_function("create_site", [&mod_state](const std::string name,
                                                   u_int width, u_int height,
                                                   bool make_current = false) {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    auto site = world->entity(name.c_str())
                    .set<Site>({width, height})
                    .set<Transform>({{0, 50}, {}})
                    .add<ConstructionSiteNeedsUpdating>();
    if (make_current) {
      site.add<CurrentSite>();
    }
    return site;
  });

  helpers.set_function(
      "create_building",
      [&mod_state](const std::string &name, const std::string &prefab, u_int x,
                   u_int y, flecs::entity site) -> flecs::entity {
        auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
        std::string prefabName = "Prefabs::Buildings::";
        prefabName.append(prefab);
        auto prefabE = world->lookup(prefabName.c_str());
        if (!prefabE.is_valid()) {
          spdlog::error("Prefab {} does not exist", prefabName);
          return flecs::entity();
        }

        auto entity = world->entity(name.c_str())
                          .is_a(world->lookup(prefabName.c_str()))
                          .set<SiteLocation>({x, y})
                          .child_of(site);
        return entity;
      });

  helpers.set_function("create_effect", [&mod_state](const std::string &name,
                                                     flecs::entity source) {
    auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
    auto effect = world->entity(name.c_str())
                      .add<Effect>()
                      .child_of((world->lookup("Effects")));
    if (source.is_valid()) {
      source.add<HasEffect>(effect);
    }
    return effect;
  });

  helpers.set_function(
      "add_modifier", [&mod_state](flecs::entity effect, Modifier mod) {
        auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
        if (!effect.is_valid()) {
          return flecs::entity();
        }
        auto modifier = world->entity().child_of(effect).set<Modifier>(mod);
        return modifier;
      });
}

void load_script_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto script_ns = solcorp_ns["script"].get_or_create<sol::table>();
  auto handlers_ns = script_ns["handlers"].get_or_create<sol::table>();
}

void load_entity_usertype(sol::state &mod_state) {
  sol::usertype<flecs::entity> flecs_entity =
      mod_state.new_usertype<flecs::entity>("entity");
  flecs_entity["id"] = &flecs::entity::id;
  flecs_entity["destruct"] = &flecs::entity::destruct;
  flecs_entity["is_alive"] = &flecs::entity::is_alive;
  flecs_entity["name"] = &flecs::entity::name;
  flecs_entity["symbol"] = &flecs::entity::symbol;
  flecs_entity["enabled"] = [](flecs::entity &e) { return e.enabled(); };
  flecs_entity["enable"] = [](flecs::entity &e) { return e.enable(); };
  flecs_entity["disable"] = [](flecs::entity &e) { return e.disable(); };
  flecs_entity["child_of"] = [](flecs::entity &e, flecs::entity &rhs) {
    e.child_of(rhs);
  };
  flecs_entity["is_a"] = [](flecs::entity &e, const std::string &name) {
    auto world = e.world();
    std::string prefabPath = fmt::format("Prefabs::{}", name);
    auto prefab = world.lookup(prefabPath.c_str());
    if (!prefab.is_valid()) {
      spdlog::error("Cannot instantiate {} - Does not exist", prefabPath);
      return;
    }
    e.is_a(prefab);
  };

  flecs_entity["has"] = [](flecs::entity &e, const std::string &name) {
    auto world = e.world();
    auto compEntity = world.lookup(name.c_str());
    bool retval = e.has(compEntity);
    return retval;
  };

  flecs_entity["remove"] = [](flecs::entity &e, const std::string &name) {
    auto world = e.world();
    auto compEntity = world.lookup(name.c_str());
    bool retval = e.remove(compEntity);
    return retval;
  };
}
