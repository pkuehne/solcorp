#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemBuildingUpdateManufacuringProgress", "[system]") {
  flecs::world world = flecs::world();
  world.import <SiteModule>();
  flecs::entity building = world.entity().set<Manufacturing>({});
  auto &manuf = building.get_mut<Manufacturing>();
  manuf.available_effort = 20;

  GIVEN("An invalid rocket in a Manufacturing line") {
    WHEN("manufacturing update is run") {
      systemBuildingUpdateManufacuringProgress(building, manuf);
      THEN("no crash occurs") {
        //
      }
    }
  }

  GIVEN("A finished rocket") {
    flecs::entity rocket = world.entity().child_of(building);
    WHEN("manufacturing update is run") {
      systemBuildingUpdateManufacuringProgress(building, manuf);
      THEN("The manufacturing line is not cleared") {
        REQUIRE(rocket.parent() == building);
      }
      THEN("The rocket is no longer under construction") {
        REQUIRE(rocket.has<Construction>() == false);
      }
    }
  }

  GIVEN("An unfinished rocket with more manfufacturing effort needed") {
    flecs::entity rocket =
        world.entity()
            .set<Construction>({.effort_remaining = 100, .effort_total = 100})
            .child_of(building);
    WHEN("manufacturing update is run") {
      systemBuildingUpdateManufacuringProgress(building, manuf);
      THEN("the construction effort is reduced") {
        REQUIRE(rocket.parent() == building);
        REQUIRE(rocket.get<Construction>().effort_remaining == 80);
        //
      }
    }
  }

  GIVEN("An unfinished rocket with very little manfufacturing effort needed") {
    flecs::entity rocket =
        world.entity()
            .set<Construction>({.effort_remaining = 20, .effort_total = 100})
            .child_of(building);
    WHEN("manufacturing update is run") {
      systemBuildingUpdateManufacuringProgress(building, manuf);
      THEN("the rocket is completed") {
        REQUIRE(rocket.parent() == building);
        REQUIRE(rocket.has<Construction>() == false);
        //
      }
    }
  }

  GIVEN("An unfinished rocket with no manfufacturing effort needed") {
    flecs::entity rocket =
        world.entity()
            .set<Construction>({.effort_remaining = 0, .effort_total = 100})
            .child_of(building);
    WHEN("manufacturing update is run") {
      systemBuildingUpdateManufacuringProgress(building, manuf);
      THEN("the rocket is completed") {
        REQUIRE(rocket.parent() == building);
        REQUIRE(rocket.has<Construction>() == false);
        //
      }
    }
  }
}
