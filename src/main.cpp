#include "modules/engine/engine.h"
#include "modules/lua/lua.h"
#include "modules/main_menu/main_menu.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/staff/staff.h"
#include "modules/stats/stats.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

int main(void) {
  auto logger = spdlog::basic_logger_mt("solcorp", "./solcorp.log", true);
  spdlog::set_default_logger(logger);
  logger->set_level(spdlog::level::debug);
  logger->flush_on(spdlog::level::debug);

  flecs::world world;

  load_config_file();

  world.import <LuaModule>();
  world.import <EngineModule>();
  world.import <StatsModule>();
  world.import <SimulationModule>();
  world.import <MainMenuModule>();
  world.import <SiteModule>();
  world.import <RocketLaunchModule>();
  world.import <StaffModule>();

  // Main Loop
  logger->info("Starting");
  world.set_target_fps(60);
  //   ecs_log_set_level(0);
  int rcode = world.app().enable_stats().enable_rest().run();
  return rcode;
}
