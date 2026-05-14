#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemAutoStartNextBuild", "[system]") {
  flecs::world world;
  world.import<SimulationModule>();
  world.import<SiteModule>();
  world.import<RocketModule>();

  auto makeLine = [&](bool auto_build) {
    return world.entity().set<Manufacturing>(
        {.available_effort = 50, .auto_build_next = auto_build});
  };

  auto countRocketChildren = [](flecs::entity line) {
    int count = 0;
    line.children([&](flecs::entity child) {
      if (child.has<Rocket>())
        count++;
    });
    return count;
  };

  GIVEN("auto_build_next is false") {
    auto prefab = world.prefab().add<Rocket>();
    auto line = makeLine(false);
    line.add<ManufacturingLineTemplate>(prefab);
    world.get_mut<Company>().balance = 10'000'000;

    WHEN("systemAutoStartNextBuild runs") {
      systemAutoStartNextBuild(line, line.get<Manufacturing>());

      THEN("no rocket is started") {
        CHECK(countRocketChildren(line) == 0);
      }
    }
  }

  GIVEN("auto_build_next is true but no template is set") {
    auto line = makeLine(true);
    world.get_mut<Company>().balance = 10'000'000;

    WHEN("systemAutoStartNextBuild runs") {
      systemAutoStartNextBuild(line, line.get<Manufacturing>());

      THEN("no rocket is started") {
        CHECK(countRocketChildren(line) == 0);
      }
    }
  }

  GIVEN("auto_build_next is true, template set, line already has a rocket") {
    auto prefab = world.prefab().add<Rocket>();
    auto line = makeLine(true);
    line.add<ManufacturingLineTemplate>(prefab);
    world.get_mut<Company>().balance = 10'000'000;

    world.entity().add<Rocket>().child_of(line);

    WHEN("systemAutoStartNextBuild runs") {
      systemAutoStartNextBuild(line, line.get<Manufacturing>());

      THEN("no new rocket is started") {
        CHECK(countRocketChildren(line) == 1);
      }
    }
  }

  GIVEN("auto_build_next is true, template set, line is free, insufficient balance") {
    auto prefab = world.prefab().add<Rocket>();
    auto line = makeLine(true);
    line.add<ManufacturingLineTemplate>(prefab);
    world.get_mut<Company>().balance = 0;

    WHEN("systemAutoStartNextBuild runs") {
      systemAutoStartNextBuild(line, line.get<Manufacturing>());

      THEN("no rocket is started") {
        CHECK(countRocketChildren(line) == 0);
      }
    }
  }

  GIVEN("auto_build_next is true, template set, line is free, sufficient balance") {
    auto prefab = world.prefab().add<Rocket>();
    auto line = makeLine(true);
    line.add<ManufacturingLineTemplate>(prefab);
    world.get_mut<Company>().balance = 10'000'000;

    WHEN("systemAutoStartNextBuild runs") {
      systemAutoStartNextBuild(line, line.get<Manufacturing>());

      THEN("a rocket is created as a child of the line under construction") {
        REQUIRE(countRocketChildren(line) == 1);
        flecs::entity created = flecs::entity::null();
        line.children([&](flecs::entity child) {
          if (child.has<Rocket>())
            created = child;
        });
        REQUIRE(created.is_valid());
        CHECK(created.get<Rocket>().state == RocketStateId::UnderConstruction);
        CHECK(created.has<EffortRequired>());
      }

      THEN("the cost is deducted from the company balance") {
        CHECK(world.get<Company>().balance < 10'000'000);
      }
    }
  }
}
