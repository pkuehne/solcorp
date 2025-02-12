#include "modules/engine/engine.h"
#include "modules/engine/render.h"
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

  world.component<std::string>()
      .opaque(flecs::String) // Opaque type that maps to string
      .serialize([](const flecs::serializer *s, const std::string *data) {
        const char *str = data->c_str();
        return s->value(flecs::String, &str); // Forward to serializer
      })
      .assign_string([](std::string *data, const char *value) {
        *data = value; // Assign new value to std::string
      });

  world.import <EngineModule>();
  world.import <StatsModule>();
  world.import <SimulationModule>();
  world.import <MainMenuModule>();
  world.import <SiteModule>();
  world.import <RocketLaunchModule>();
  world.import <StaffModule>();

  world.system("Load Textures")
      .kind(flecs::OnStart)
      .immediate()
      .run([](flecs::iter &iter) {
        spdlog::debug("Loading textures");
        auto world = iter.world();
        auto root = world.entity("Textures");

        world.entity("Buildings")
            .set<Texture>(loadTexture("textures/solcorp_buildings.png", world))
            .child_of(root);
      });

  world.system("TileMap Creation")
      .kind(flecs::OnStart)
      .immediate()
      .run([](flecs::iter &iter) {
        auto world = iter.world();
        auto texture = world.lookup("Textures::Buildings");
        const uint tileSize = 32;
        world.entity("BuildingTileMap")
            .set<TileMap>(tileMapFromTexture(texture, tileSize));
      });

  world.system("Prefab Creation")
      .kind(flecs::OnStart)
      .immediate()
      .run([](flecs::iter &iter) {
        spdlog::debug("Setting up prefabs");

        auto world = iter.world();
        auto tm = world.lookup("BuildingTileMap");

        auto building = world.lookup("SiteModule::Building");
        auto prefabs = world.entity("Prefabs");
        auto buildings = world.entity("Buildings").child_of(prefabs);

        world.prefab("Factory")
            .is_a(building)
            .child_of(buildings)
            .set<Sprite>(spriteFromTileMap(tm, 2))
            .emplace<Manufacturing>(Manufacturing(2))
            .set<Storage>({});
        world.prefab("Storage Hall")
            .is_a(building)
            .child_of(buildings)
            .set<Sprite>(spriteFromTileMap(tm, 2))
            .set<Storage>({});
        world.prefab("Launchpad")
            .is_a(building)
            .child_of(buildings)
            .set<Sprite>(spriteFromTileMap(tm, 3))
            .set<Launchpad>({});
        world.prefab("Office Building")
            .is_a(building)
            .child_of(buildings)
            .set<Sprite>(spriteFromTileMap(tm, 1))
            .set<Office>({});
      });

  world.system("Site Creation")
      .kind(flecs::OnStart)
      .immediate()
      .run([](flecs::iter &iter) {
        auto world = iter.world();
        auto site = world.entity("Cape Canaveral")
                        .set<Site>({10, 10})
                        .add<CurrentSite>()
                        .set<Transform>({{0, 50}, {}});
        site.add<constructionSiteNeedsUpdating>();

        world.entity("Manufacturing A")
            .is_a(world.lookup("Prefabs::Buildings::Factory"))
            .set<SiteLocation>({1, 1})
            .child_of(site);
        world.entity("Storage Hall 1")
            .is_a(world.lookup("Prefabs::Buildings::Storage Hall"))
            .set<SiteLocation>({0, 0})
            .child_of(site);
        world.entity("Main Launchpad")
            .is_a(world.lookup("Prefabs::Buildings::Launchpad"))
            .set<SiteLocation>({1, 0})
            .child_of(site);
        world.entity("North Building")
            .is_a(world.lookup("Prefabs::Buildings::Office Building"))
            .set<SiteLocation>({2, 0})
            .child_of(site);

        // Add effects
        auto effect = world.entity("Better Concrete").add<Effect>();
        world.entity().child_of(effect).set<Modifier>({"max-weight", 0.0, 1.2});
        site.add<HasEffect>(effect);

        auto effect2 = world.entity("Cracks detected").add<Effect>();
        world.entity().child_of(effect2).set<Modifier>(
            {"max-weight", 0.0, 0.3});
        site.add<HasEffect>(effect2);

        auto effect3 = world.entity("Reinforcing Struts").add<Effect>();
        world.entity().child_of(effect3).set<Modifier>(
            {"max-weight", 500.0, 1.0});
        site.add<HasEffect>(effect3);
      });

  // Main Loop
  logger->info("Starting");
  world.set_target_fps(60);
  //   ecs_log_set_level(0);
  int rcode = world.app().enable_stats().enable_rest().run();

  return rcode;
}
