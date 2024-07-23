#include "components.h"
#include "graphics.h"
#include "gui.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"
#include "systems.h"
#include "systems/building_systems.h"
// #include "systems/rocket_system.h"

void registerResources(flecs::world &world) {
  world.set<PrefabResource>({
      world.prefab("Rocket").add<Rocket>().set<CargoHold>({1000}), // Rocket
  });
  // Find all Persons that are TeamMember of a $team, where the $team is a
  // TeamMember of anything
  // i.e. Match any person with a team that has another team above it.
  // team_members.each([](flecs::iter &it, size_t, Person &p)
  //                   { std::cout << p.first_name << " " << p.last_name << " is
  //                   a member of "
  //                               << it.get_var("team").name() << std::endl;
  //                               });
  world.set<QueryResource>({
      world.rule_builder<Person>()
          .with<Employee>()
          .with<TeamMember>()
          .second("$team")
          .with<TeamMember>(flecs::Any)
          .src("$team")
          .build(), // team_members
  });
  world.set<GuiResource>({});
}

void registerSystems(flecs::world &world) {
  auto game = world.get<GameResource>();

  auto preFramePhase = world.entity("Phase.PreFrame").add(flecs::Phase);
  auto validatePhase = world.entity("Phase.Validate")
                           .add(flecs::Phase)
                           .depends_on(preFramePhase);
  auto PostValidatePhase = world.entity("Phase.PostValidate")
                               .add(flecs::Phase)
                               .depends_on(validatePhase);
  auto UpdatePhase = world.entity("Phase.Update")
                         .add(flecs::Phase)
                         .depends_on(PostValidatePhase);
  auto RenderPhase =
      world.entity("Phase.Render").add(flecs::Phase).depends_on(UpdatePhase);
  // auto CleanupPhase =
  //     world.entity("Phase.Cleanup").add(flecs::Phase).depends_on(RenderPhase);
  // auto PostFramePhase = world.entity("Phase.PostFrame")
  //                           .add(flecs::Phase)
  //                           .depends_on(CleanupPhase);

  // Startup Systems
  //   world.system<>("company_generator_system")
  //       .kind(flecs::OnStart)
  //       .iter(company_generator_system);
  //   world.system<>("rocket_generator_system")
  //       .kind(flecs::OnStart)
  //       .iter(rocket_generator_system);
  // PreFrame

  // Update Phase

  world.system<GameResource, Renderer>("Event Handling")
      .term_at(1)
      .singleton()
      .term_at(2)
      .singleton()
      .kind(preFramePhase)
      .iter(systemEventHandling);

  world.system<GuiResource>("Update UI")
      .term_at(1)
      .singleton()
      .kind(preFramePhase)
      .iter(systemUpdateUI);

  // Simulator Systems
  world.system<GameResource>("Update Simulation Date")
      .tick_source(game->sim_speed)
      .term_at(1)
      .singleton()
      .kind(UpdatePhase)
      .iter(systemUpdateSimDate);

  world.system<Manufacturing>("Update Construction")
      .tick_source(game->sim_speed)
      .kind(UpdatePhase)
      .each(systemBuildingUpdateConstruction);

  // Render Phase
  world.system<const Renderer>("Render Begin")
      .term_at(1)
      .singleton()
      .kind(RenderPhase)
      .iter(systemRenderClear);

  world.system<const Sprite, const Position, const Renderer>("Render Sprites")
      .term_at(3)
      .singleton()
      .kind(RenderPhase)
      .iter(systemRenderSprite);

  world.system<const Renderer>("Render UI")
      .term_at(1)
      .singleton()
      .kind(RenderPhase)
      .iter(systemRenderUI);

  world.system<const Renderer>("Render End")
      .term_at(1)
      .singleton()
      .kind(RenderPhase)
      .iter(systemRenderPresent);

  // Cleanup
  // PostFrame

  //   world.system<Rocket>("rocket_print_system")
  //       .tick_source(game->sim_speed)
  //       .each(rocket_print_system);
  //   world.system<Rocket>("rocket_launch_scheduler_system")
  //       .interval(3.0f)
  //       .each(rocket_launch_scheduler_system);
  //   world.system<PlanetaryLaunch>("launch_decrementer_system")
  //       .tick_source(game->sim_speed)
  //       .each(launch_decrementer_system);

  //   // Continuous Systems
  //   world.system<GameResource>("render_system")
  //       .term_at(1)
  //       .singleton()
  //       .kind(flecs::OnStore)
  //       .iter(render_system);

  //   world.system<PlanetaryLaunch>("launch_end_system")
  //       .each([](flecs::iter &it, size_t i, PlanetaryLaunch &l) {
  //         auto entity = it.entity(i);
  //         launch_end_system(entity, l);
  //       });
}

void prepareGraphics(flecs::world &world) {
  initialiseGraphics(world);
  initialiseGUI(world);

  Texture t = loadTexture("../../textures/solcorp_buildings.png", world);
  t.cols = 1;
  t.rows = 4;
  world.entity("building_texture").set<Texture>(t);
}

int main(void) {
  auto logger = spdlog::basic_logger_mt("solcorp", "./solcorp.log", true);
  spdlog::set_default_logger(logger);
  logger->set_level(spdlog::level::debug);
  logger->flush_on(spdlog::level::info);

  flecs::world world;

  logger->info("Assigning GameResource");
  world.set<GameResource>({
      world.timer("SimTimer").interval(0.5f).disable(), // sim_speed, start
                                                        // paused
      0,                                                // Day
  });

  registerResources(world);
  registerComponents(world);
  registerSystems(world);
  prepareGraphics(world);

  auto site =
      world.entity("cape_canaveral").set<Site>({"Cape Canaveral", 10, 10});
  world.entity()
      .set<Building>({"Storage Hall 1"})
      .set<Storage>({})
      .set<SiteLocation>({0, 0})
      .set<Position>({0, 1})
      .set<Sprite>({"building_texture", 2})
      .child_of(site);
  world.entity()
      .set<Building>({"Launchpad"})
      .set<Launchpad>({})
      .set<SiteLocation>({1, 0})
      .set<Position>({1, 1})
      .set<Sprite>({"building_texture", 3})
      .child_of(site);
  world.entity()
      .set<Building>({"North Building"})
      .set<Office>({})
      .set<SiteLocation>({2, 0})
      .set<Position>({2, 1})
      .set<Sprite>({"building_texture", 1})
      .child_of(site);
  world.entity()
      .set<Building>({"Manufacturing A"})
      .set<Manufacturing>({})
      .set<Storage>({})
      .set<SiteLocation>({1, 1})
      .set<Position>({1, 2})
      .set<Sprite>({"building_texture", 2})
      .child_of(site);

  world.get_mut<GuiResource>()->site_window.siteEntity = site;

  // Main Loop
  logger->info("Starting");
  world.import <flecs::monitor>();
  world.set_target_fps(60);
  int rcode = world.app().enable_rest().run();

  return rcode;
}
