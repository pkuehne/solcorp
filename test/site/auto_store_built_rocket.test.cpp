#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemAutoStoreBuiltRocket", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  auto makeLine = [&](bool auto_store) {
    return world.entity().set<Manufacturing>(
        {.available_effort = 50, .auto_store = auto_store});
  };

  auto makeStorage = [&]() { return world.entity().add<Storage>(); };

  auto addStoredRocketChild = [&](flecs::entity line) {
    auto rocket = world.entity().add<Rocket>().child_of(line);
    rocket.get_mut<Rocket>().state = RocketStateId::Stored;
    return rocket;
  };

  auto callSystem = [](flecs::entity rocketE, const Manufacturing &mfg) {
    systemAutoStoreBuiltRocket(rocketE, rocketE.get_mut<Rocket>(), mfg);
  };

  GIVEN("auto_store is false") {
    auto storage = makeStorage();
    auto line = makeLine(false);
    line.add<ManufacturingLineStorage>(storage);
    auto rocket = addStoredRocketChild(line);

    WHEN("systemAutoStoreBuiltRocket runs") {
      callSystem(rocket, line.get<Manufacturing>());

      THEN("the rocket is not moved") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
        CHECK(rocket.parent() == line);
      }
    }
  }

  GIVEN("auto_store is true but rocket is under construction") {
    auto storage = makeStorage();
    auto line = makeLine(true);
    line.add<ManufacturingLineStorage>(storage);

    auto rocket = world.entity().add<Rocket>().child_of(line);
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;

    WHEN("systemAutoStoreBuiltRocket runs") {
      callSystem(rocket, line.get<Manufacturing>());

      THEN("the rocket is not moved") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::UnderConstruction);
        CHECK(rocket.parent() == line);
      }
    }
  }

  GIVEN("auto_store is true, rocket is stored, but no storage destination is "
        "set") {
    auto line = makeLine(true);
    auto rocket = addStoredRocketChild(line);

    WHEN("systemAutoStoreBuiltRocket runs") {
      callSystem(rocket, line.get<Manufacturing>());

      THEN("the rocket is not moved") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
        CHECK(rocket.parent() == line);
      }
    }
  }

  GIVEN(
      "auto_store is true, a stored rocket exists, and storage is configured") {
    auto storage = makeStorage();
    auto line = makeLine(true);
    line.add<ManufacturingLineStorage>(storage);
    auto rocket = addStoredRocketChild(line);

    WHEN("systemAutoStoreBuiltRocket runs") {
      callSystem(rocket, line.get<Manufacturing>());

      THEN("the rocket transitions to Moving") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Moving);
      }

      THEN("AutoStoreBlocked is cleared") {
        CHECK(!line.has<AutoStoreBlocked>());
      }
    }
  }

  GIVEN("auto_store is true but the rocket still has EffortRequired "
        "(deferred-command race)") {
    auto storage = makeStorage();
    auto line = makeLine(true);
    line.add<ManufacturingLineStorage>(storage);

    // Simulate the race: state is Stored (immediate write from
    // RocketCompleteBuildAction) but EffortRequired removal is still pending
    // (deferred structural change).
    auto rocket = addStoredRocketChild(line);
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});

    WHEN("systemAutoStoreBuiltRocket runs") {
      callSystem(rocket, line.get<Manufacturing>());

      THEN("the rocket is not moved") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
        CHECK(rocket.parent() == line);
      }

      THEN("the line is marked as blocked") {
        CHECK(line.has<AutoStoreBlocked>());
      }
    }

    WHEN("systemAutoStoreBuiltRocket runs repeatedly") {
      callSystem(rocket, line.get<Manufacturing>());
      callSystem(rocket, line.get<Manufacturing>());
      callSystem(rocket, line.get<Manufacturing>());

      THEN("the line stays blocked without spawning duplicate notifications") {
        CHECK(line.has<AutoStoreBlocked>());
      }
    }
  }

  GIVEN("a previously blocked line where EffortRequired is now gone") {
    auto storage = makeStorage();
    auto line = makeLine(true);
    line.add<ManufacturingLineStorage>(storage);
    line.add<AutoStoreBlocked>();
    auto rocket = addStoredRocketChild(line);

    WHEN("systemAutoStoreBuiltRocket runs") {
      callSystem(rocket, line.get<Manufacturing>());

      THEN("the blocked tag is cleared and the rocket begins moving") {
        CHECK(!line.has<AutoStoreBlocked>());
        CHECK(rocket.get<Rocket>().state == RocketStateId::Moving);
      }
    }
  }
}
