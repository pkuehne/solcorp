#include "modules/base/base.h"
#include "modules/simulation/simulation.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

extern void systemUpdateSimDate(Game &game);

SCENARIO("systemUpdateSimDate", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();

  GIVEN("A game at day 0") {
    Game *game = world.try_get_mut<Game>();
    REQUIRE(game->day == 0);
    WHEN("The system is run") {
      systemUpdateSimDate(*game);
      THEN("The day updates to 1") { REQUIRE(game->day == 1); }
    }
  }
}
