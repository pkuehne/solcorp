#include "systems.h"
#include "modules/lua/lua.h"

void mod_on_start(flecs::entity e, Mod &mod) {
  auto world = e.world();
  world.defer_suspend();
  run_mod_handler(mod, world, "on_start");
  world.defer_resume();
}

void mod_on_frame(flecs::entity e, Mod &mod) {
  auto world = e.world();
  run_mod_handler(mod, world, "on_frame");
}

void mod_on_update(flecs::entity e, Mod &mod) {
  auto world = e.world();
  run_mod_handler(mod, world, "on_update");
}