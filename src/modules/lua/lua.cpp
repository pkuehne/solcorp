#include "lua.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "modules/lua/entity.h"
#include "modules/lua/helpers.h"
#include "modules/lua/logging.h"
#include "modules/lua/lua_registry.h"
#include "modules/lua/mod_content.h"
#include "modules/lua/mod_manifest.h"
#include "modules/lua/systems.h"
#include "spdlog/spdlog.h"
#include <cstdlib>
#include <filesystem>
#include <flecs.h>
#include <flecs/addons/cpp/world.hpp>
#include <unordered_map>
#include <vector>

void load_mod_state(lua_State *L);
void load_script_namespace(lua_State *L);
void load_all_mods(flecs::world &world);
flecs::entity load_mod(flecs::world &world, const std::filesystem::path &path,
                       const ModManifest &manifest, int load_order);

// The resolved mod load order, captured once at load time. run_on_every_mod
// iterates this rather than re-deriving the order (flecs child iteration order
// is not the load order) on every call.
struct ModLoadOrder {
  std::vector<flecs::entity> mods; // dependencies-first
};

LuaModule::LuaModule(flecs::world &world) {
  // Register components
  world.component<Mod>()
      .member("id", &Mod::id)
      .member("name", &Mod::name)
      .member("version", &Mod::version)
      .member("description", &Mod::description)
      .member("author", &Mod::author)
      .member("load_order", &Mod::load_order); //
  //.member("state", &Mod::state); //
  world.component<ReloadPrefabsRequest>();

  // Load mods
  load_all_mods(world);

  // Apply each mod's data-driven content (textures, building prefabs) once the
  // ECS prefab nodes exist. Runs before mod on_start so handlers can rely on
  // building prefabs being registered.
  world.system("Load Mod Content")
      .kind(PostStartPhase)
      .immediate()
      .run([](flecs::iter &it) {
        flecs::world world = it.world();
        loadModContent(world);
      });

  world.system<Mod>("Mod on_start Event")
      .kind(PostStartPhase)
      .immediate()
      .each(mod_on_start);

  // Consume a reload request raised from elsewhere (e.g. the developer window's
  // "Reload Prefabs" button). Matching the singleton tag means the body only
  // runs on a frame where a reload was requested. immediate() runs it outside
  // readonly mode, where loadModContent's structural changes are legal.
  world.system("Reload Mod Content")
      .with<ReloadPrefabsRequest>()
      .kind(flecs::OnUpdate)
      .immediate()
      .each([](flecs::entity e) {
        flecs::world world = e.world();
        world.remove<ReloadPrefabsRequest>();
        loadModContent(world);
        spdlog::info("Reloaded mod content (textures and building prefabs)");
        instantiateNotification(
            world, "Prefabs Reloaded",
            "Reloaded mod textures and building prefabs from disk.", {},
            NotificationSeverity::Medium);
      });

  world.system<Mod>("Mod on_frame Event")
      .kind(flecs::OnUpdate)
      .each(mod_on_frame);

  auto sim = world.get<Simulation>();
  world.system<Mod>("Mod on_update Event")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(mod_on_update);
}

// Logs a fatal mod-loading error and aborts: an unresolvable mod graph leaves
// the game in an undefined content state, so we fail loudly rather than limp
// on.
[[noreturn]] static void fatal_mod_error(const std::string &message) {
  spdlog::critical("{}", message);
  spdlog::default_logger()->flush();
  std::exit(EXIT_FAILURE);
}

void load_all_mods(flecs::world &world) {
  auto scope = world.set_scope(0);
  world.entity("Mods");
  world.set_scope(scope);

  std::filesystem::path mod_path = "./mods/";
  if (!std::filesystem::exists(mod_path)) {
    std::filesystem::create_directory(mod_path);
  }

  // A directory is a mod iff it contains a mod.lua manifest. init.lua is
  // optional (e.g. a graphics-only mod has no event handlers to register).
  std::vector<ModManifest> manifests;
  for (const auto &entry : std::filesystem::directory_iterator(mod_path)) {
    if (!entry.is_directory()) {
      continue;
    }
    if (!std::filesystem::exists(entry.path() / "mod.lua")) {
      continue;
    }
    try {
      manifests.push_back(readModManifest(entry.path()));
    } catch (const ModDependencyError &e) {
      fatal_mod_error(std::string("Mod manifest error: ") + e.what());
    }
  }

  std::vector<std::string> order;
  try {
    order = resolveLoadOrder(manifests);
  } catch (const ModDependencyError &e) {
    fatal_mod_error(std::string("Mod dependency resolution failed: ") +
                    e.what());
  }

  std::unordered_map<std::string, const ModManifest *> by_id;
  for (const auto &manifest : manifests) {
    by_id[manifest.id] = &manifest;
  }

  // Register before set<>: flecs is built with FLECS_CPP_NO_AUTO_REGISTRATION,
  // so storing an unregistered component asserts.
  world.component<ModLoadOrder>();

  ModLoadOrder load_order;
  int index = 0;
  for (const auto &id : order) {
    flecs::entity mod = load_mod(world, mod_path / id, *by_id[id], index++);
    if (mod.is_valid()) {
      load_order.mods.push_back(mod);
    }
  }
  world.set<ModLoadOrder>(std::move(load_order));
}

flecs::entity load_mod(flecs::world &world, const std::filesystem::path &path,
                       const ModManifest &manifest, int load_order) {
  const std::string &mod_name = manifest.id;
  spdlog::info("Loading mod {} (v{}, order {})", mod_name, manifest.version,
               load_order);
  auto mods = world.lookup("Mods");
  auto entity = world.entity(mod_name.c_str()).child_of(mods);
  auto &mod = entity.ensure<Mod>();
  mod.id = mod_name;
  mod.name = manifest.name;
  mod.version = manifest.version;
  mod.description = manifest.description;
  mod.author = manifest.author;
  mod.load_order = load_order;
  mod.state = luaL_newstate();
  lua_set_mod_name(mod.state, mod_name);

  lua_newtable(mod.state);
  lua_setglobal(mod.state, "solcorp");

  load_mod_state(mod.state);

  auto init_file = path / "init.lua";
  if (!std::filesystem::exists(init_file)) {
    // Graphics-only mod: no init.lua to run, but the mod is still loaded.
    return entity;
  }

  if (luaL_loadfile(mod.state, init_file.string().c_str()) != LUA_OK ||
      lua_pcall(mod.state, 0, LUA_MULTRET, 0) != LUA_OK) {
    const char *err = lua_tostring(mod.state, -1);
    spdlog::error("Failed to load {}: {}", init_file.string(),
                  err ? err : "(unknown error)");
    lua_pop(mod.state, 1);
    entity.destruct();
    return {};
  }

  run_mod_handler(mod, world, "on_init");
  return entity;
}

/**
 * @brief Runs a callback on every mod
 *
 * @param world the flecs world
 * @param func The function to call
 */
void run_on_every_mod(flecs::world &world, const ModStateCallback &func) {
  // Mods are loaded by LuaModule; modules imported before it (or test worlds
  // that never load mods) have no Mods entity, in which case there is nothing
  // to run and ModLoadOrder is not yet registered.
  if (!world.lookup("Mods").is_valid()) {
    return;
  }
  // Visit mods in resolved dependency order so per-mod passes (component/enum
  // registration, etc.) run dependencies-first, matching the documented merge
  // order. The order is computed once at load time (ModLoadOrder); we never
  // re-derive it here, since it cannot change after loading.
  const auto *order = world.try_get<ModLoadOrder>();
  if (order == nullptr) {
    return;
  }
  for (const flecs::entity &modE : order->mods) {
    Mod *mod = modE.try_get_mut<Mod>();
    if (mod == nullptr) {
      spdlog::error("Mod {} does not have a Mod component!",
                    modE.name().c_str());
      continue;
    }
    func(mod->state);
  }
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
    spdlog::error("{} - Could not run '{}' function: {}", mod.id, handler, err);
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
    // Register under solcorp.constants.<name> rather than as a bare global, so
    // mods (and luacheck) only need to know about the single `solcorp` global.
    lua_getglobal(L, "solcorp");             // [solcorp]
    lua_get_or_create_table(L, "constants"); // [solcorp, constants]
    lua_newtable(L);                         // [solcorp, constants, enum]
    int tbl_idx = lua_gettop(L);
    LuaEnumBuilder builder(L, tbl_idx);
    register_func(builder);
    lua_setfield(L, -2,
                 name.c_str()); // constants[name] = enum; [solcorp, constants]
    lua_pop(L, 2);              // []
  });
}

void load_script_namespace(lua_State *L) {
  lua_getglobal(L, "solcorp");
  lua_get_or_create_table(L, "script");
  lua_get_or_create_table(L, "handlers");
  lua_pop(L, 3);
}
