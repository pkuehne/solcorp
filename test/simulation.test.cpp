#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <gtest/gtest.h>

extern void systemUpdateSimDate(Game &game);

class SimulationModuleTests : public testing::Test {
protected:
  flecs::world world;

public:
  SimulationModuleTests() { world.import <SimulationModule>(); }
};

TEST_F(SimulationModuleTests, GameDayAdvancesWhenSimulationRunning) {
  // Given
  Game *game = world.try_get_mut<Game>();
  ASSERT_EQ(game->day, 0);

  // When
  systemUpdateSimDate(*game);

  // Then
  EXPECT_EQ(game->day, 1);
}
