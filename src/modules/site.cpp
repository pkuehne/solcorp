#include "site.h"
#include "components.h"

void systemBuildingUpdateConstruction(flecs::entity, Manufacturing &);

SiteModule::SiteModule(flecs::world &world) {

  flecs::entity UpdatePhase = world.lookup("Phase.Update");
  //   flecs::entity GuiPhase = world.lookup("Phase.Gui");

  // Register components
  world.component<Site>()
      .member<std::string>("name")
      .member<u_int>("width")
      .member<u_int>("height");
  world.component<Building>().member<std::string>("name");
  world.component<SiteLocation>().member<u_int>("x").member<u_int>("y");
  world.component<Manufacturing>();
  world.component<Storage>();
  world.component<Office>();
  world.component<Launchpad>();

  // Register Systems
  auto game = world.get<GameResource>();

  world.system<Manufacturing>("Update Construction")
      .tick_source(game->sim_speed)
      .kind(UpdatePhase)
      .each(systemBuildingUpdateConstruction);
}

void systemBuildingUpdateConstruction(flecs::entity entity,
                                      Manufacturing &manufacturing) {
  flecs::world world = entity.world();

  entity.children([&](flecs::entity r) {
    Construction *construction = r.get_mut<Construction>();
    if (!construction)
      return;
    if (construction->effort_remaining == 0) {
      r.remove<Construction>();
      return;
    }
    if (manufacturing.available_effort > construction->effort_remaining) {
      construction->effort_remaining = 0;
    } else {
      construction->effort_remaining -= manufacturing.available_effort;
    }
  });
}
