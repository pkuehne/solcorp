#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemCreateSitePrefabs", "[on_start][system]") {
  flecs::world world;
  world.import<SiteModule>();

  GIVEN("An empty world") {
    auto system = world.system("Create Site Prefabs")
                      .kind(flecs::OnStart)
                      .immediate()
                      .run(systemCreateSitePrefabs);
    WHEN("The system is run") {
      system.run();
      THEN("The Prefabs::Buildings node is created") {
        auto node = world.lookup("Prefabs::Buildings");
        CHECK(node.is_valid());
      }
      THEN("The Prefabs::Core node contains a Building prefab") {
        auto node = world.lookup("Prefabs::Core::Building");
        CHECK(node.is_valid());
        CHECK(node.has<Building>());
      }
      THEN("The Prefabs::Core node contains a ConstructionSite prefab") {
        auto node = world.lookup("Prefabs::Core::ConstructionSite");
        CHECK(node.is_valid());
        CHECK(node.has<ConstructionSite>());
      }
      THEN("The Prefabs::Facilities node is created") {
        auto node = world.lookup("Prefabs::Facilities");
        CHECK(node.is_valid());
      }
      THEN("The Prefabs::Core node contains a Facility prefab") {
        auto node = world.lookup("Prefabs::Core::Facility");
        CHECK(node.is_valid());
        CHECK(node.has<Facility>());
      }
    }
  }
}
