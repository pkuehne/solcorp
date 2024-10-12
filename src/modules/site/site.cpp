#include "site.h"
#include "flecs/addons/cpp/entity.hpp"
#include "modules/input/input.h"
#include "modules/phase/phase.h"
#include "modules/render/render.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"

void systemBuildingUpdateConstruction(flecs::entity, Manufacturing &);
void systemShowBuildingWindow(flecs::entity, Transform &, const MouseUp &);
void systemDrawBuildingWindow(flecs::entity winE, BuildingWindow &win);

SiteModule::SiteModule(flecs::world &world) {

  world.import <PhaseModule>();
  world.import <SimulationModule>();
  world.import <RocketLaunchModule>();

  // Register components
  world.component<Site>().member<u_int>("width").member<u_int>("height");
  world.component<Building>();
  world.component<SiteLocation>().member<u_int>("x").member<u_int>("y");
  world.component<Manufacturing>();
  world.component<Storage>();
  world.component<Office>();
  world.component<Launchpad>();
  world.component<BuildingWindow>().member<flecs::entity>("buildingE");

  // Register Systems
  auto sim = world.get<Simulation>();

  world.system<Manufacturing>("Update Construction")
      .tick_source(sim->speed)
      .kind(UpdatePhase)
      .each(systemBuildingUpdateConstruction);

  world.system<BuildingWindow>("Draw Building Window")
      .kind(GuiPhase)
      .each(systemDrawBuildingWindow);

  world.system<Transform, const MouseUp>("Open Building Window")
      .with<Building>()
      .term_at(1)
      .singleton()
      .kind(ValidatePhase)
      .each(systemShowBuildingWindow);
}

void systemShowBuildingWindow(flecs::entity e, Transform &t,
                              const MouseUp &mouse) {
  // We know from the query that this is a Building
  int tileSize = 32; // SOL-39
  if ((mouse.x > t.worldPosition.x && mouse.x < t.worldPosition.x + tileSize) &&
      (mouse.y > t.worldPosition.y && mouse.y < t.worldPosition.y + tileSize)) {
    showBuildingWindow(e);
  }
}

void systemBuildingUpdateConstruction(flecs::entity entity,
                                      Manufacturing &manufacturing) {
  flecs::world world = entity.world();

  for (flecs::entity &rocket : manufacturing.lines) {
    if (!rocket.is_valid()) {
      continue;
    }
    Construction *construction = rocket.get_mut<Construction>();
    if (!construction) {
      // Remove it from the manufacturing line
      rocket = flecs::entity();
      continue;
    }
    if (manufacturing.available_effort > construction->effort_remaining) {
      construction->effort_remaining = 0;
    } else {
      construction->effort_remaining -= manufacturing.available_effort;
    }
    if (construction->effort_remaining == 0) {
      rocket.remove<Construction>();
      rocket = flecs::entity();
    }
  }
}
