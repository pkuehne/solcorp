#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/lua/lua.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/staff/staff.h"
#include "modules/stats/stats.h"
#include "modules/window/window_module.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

int main() {
  auto logger = spdlog::basic_logger_mt("solcorp", "./solcorp.log", true);
  spdlog::set_default_logger(logger);
  logger->set_level(spdlog::level::debug);
  logger->flush_on(spdlog::level::debug);

  flecs::world world;

  load_config_file();

  world.import <BaseModule>();
  world.import <LuaModule>();
  world.import <EngineModule>();
  world.import <StatsModule>();
  world.import <SimulationModule>();
  world.import <WindowModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();
  world.import <StaffModule>();

  // Main Loop
  logger->info("Starting");
  world.set_target_fps(60);
  //   ecs_log_set_level(0);
  int rcode = world.app().enable_stats().enable_rest().run();
  return rcode;
}
