
#include "modules/gui/gui.h"
#include "modules/input/input.h"
#include "modules/main_menu/main_menu.h"
#include "modules/phase/phase.h"
#include "modules/render/render.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/staff/staff.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

int main(void) {
  auto logger = spdlog::basic_logger_mt("solcorp", "./solcorp.log", true);
  spdlog::set_default_logger(logger);
  logger->set_level(spdlog::level::debug);
  logger->flush_on(spdlog::level::info);

  flecs::world world;

  world.component<std::string>()
      .opaque(flecs::String) // Opaque type that maps to string
      .serialize([](const flecs::serializer *s, const std::string *data) {
        const char *str = data->c_str();
        return s->value(flecs::String, &str); // Forward to serializer
      })
      .assign_string([](std::string *data, const char *value) {
        *data = value; // Assign new value to std::string
      });

  world.import <PhaseModule>();
  world.import <SimulationModule>();
  world.import <RenderModule>();
  world.import <GuiModule>();
  world.import <InputModule>();
  world.import <MainMenuModule>();
  world.import <SiteModule>();
  world.import <RocketLaunchModule>();
  world.import <StaffModule>();

  auto site = world.entity("cape_canaveral")
                  .set<Site>({"Cape Canaveral", 10, 10})
                  .set<Transform>({{0, 50}, {}});
  world.entity()
      .set<Building>({"Storage Hall 1"})
      .set<Storage>({})
      .set<SiteLocation>({0, 0})
      .set<Transform>({{0, 0}, {}})
      .set<Sprite>({"building_texture", 2})
      .child_of(site);
  world.entity()
      .set<Building>({"Launchpad"})
      .set<Launchpad>({})
      .set<SiteLocation>({1, 0})
      .set<Transform>({{32, 0}, {}})
      .set<Sprite>({"building_texture", 3})
      .child_of(site);
  world.entity()
      .set<Building>({"North Building"})
      .set<Office>({})
      .set<SiteLocation>({2, 0})
      .set<Transform>({{64, 0}, {}})
      .set<Sprite>({"building_texture", 1})
      .child_of(site);
  world.entity()
      .set<Building>({"Manufacturing A"})
      .set<Manufacturing>({})
      .set<Storage>({})
      .set<SiteLocation>({1, 1})
      .set<Transform>({{32, 32}, {}})
      .set<Sprite>({"building_texture", 2})
      .child_of(site);

  // Main Loop
  logger->info("Starting");
  world.set_target_fps(60);
  //   ecs_log_set_level(0);
  int rcode = world.app().enable_rest().run();

  return rcode;
}
