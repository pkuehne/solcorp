#include "modules/simulation/simulation.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemGenerateSolSystem", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();

  GIVEN("The system is registered") {
    auto system = world.system("Generate Sol System")
                      .kind(flecs::OnStart)
                      .immediate()
                      .run(systemGenerateSolSystem);
    WHEN("The system is run") {
      system.run();
      THEN("The Sun exists") { REQUIRE(world.lookup("Sun").is_valid()); }
      THEN("The Earth exists as a child of the Sun") {
        REQUIRE(world.lookup("Sun::Earth").is_valid());
      }
      THEN("The Moon exists as a child of the Earth") {
        REQUIRE(world.lookup("Sun::Earth::Moon").is_valid());
      }
    }
  }
}
