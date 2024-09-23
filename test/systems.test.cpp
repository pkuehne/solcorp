// #include "components.h"
// #include "systems/rocket_system.h"
// #include <flecs.h>
// #include <gtest/gtest.h>

// class LaunchEndSystem : public testing::Test {
// protected:
//   flecs::world world;

// public:
//   LaunchEndSystem() { registerComponents(world); }
// };

// TEST_F(LaunchEndSystem, DoesNotCompleteInProgressLaunches) {
//   // Given
//   auto pl = PlanetaryLaunch{10, false};
//   auto launch = world.entity().set<PlanetaryLaunch>(pl);

//   // When
//   launch_end_system(launch, pl);

//   // Then
//   EXPECT_FALSE(pl.completed);
// }

// TEST_F(LaunchEndSystem, DoesNotCompleteIfAlreadyComplete) {
//   // Given
//   auto pl = PlanetaryLaunch{10, true};
//   auto launch = world.entity().set<PlanetaryLaunch>(pl);

//   // When
//   launch_end_system(launch, pl);

//   // Then
//   EXPECT_TRUE(pl.completed);
// }

// TEST_F(LaunchEndSystem, CompletesIfDaysZero) {
//   // Given
//   auto pl = PlanetaryLaunch{0, false};
//   auto launch = world.entity().set<PlanetaryLaunch>(pl);
//   auto rocket = world.entity();
//   launch.add<LaunchingWith>(rocket);

//   // When
//   launch_end_system(launch, pl);

//   // Then
//   EXPECT_TRUE(pl.completed);
//   EXPECT_FALSE(rocket.is_alive());
// }
