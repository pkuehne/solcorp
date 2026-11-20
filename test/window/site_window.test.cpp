#include "modules/window/site_window.h"
#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

#include <vector>

SCENARIO("buildPalette lists what a site can build", "[window][site]") {
  flecs::world world;
  world.import <SiteModule>();
  auto prefabs = world.entity("Prefabs");
  auto buildings = world.entity("Buildings").child_of(prefabs);
  auto factory = world.prefab("factory").child_of(buildings);
  auto office = world.prefab("office").child_of(buildings);
  auto gates = world.entity("Gates").child_of(prefabs);
  auto gate = world.prefab("gate").child_of(gates);
  auto empty = world.entity("Empty").child_of(prefabs);

  GIVEN("a site that can build one category") {
    auto site = world.entity().add<Buildable>(buildings);

    WHEN("its palette is built") {
      auto palette = buildPalette(site);

      THEN("there is one section with that category's prefabs in order") {
        REQUIRE(palette.size() == 1);
        CHECK(palette[0].category == buildings);
        CHECK(palette[0].prefabs == std::vector{factory, office});
      }
    }
  }

  GIVEN("a site that can build two categories") {
    auto site = world.entity().add<Buildable>(buildings).add<Buildable>(gates);

    WHEN("its palette is built") {
      auto palette = buildPalette(site);

      THEN("each category has its own section") {
        REQUIRE(palette.size() == 2);
        CHECK(palette[0].prefabs == std::vector{factory, office});
        CHECK(palette[1].prefabs == std::vector{gate});
      }
    }
  }

  GIVEN("a site whose only category is empty") {
    auto site = world.entity().add<Buildable>(empty);

    WHEN("its palette is built") {
      auto palette = buildPalette(site);

      THEN("the section is shown without prefabs") {
        REQUIRE(palette.size() == 1);
        CHECK(palette[0].prefabs.empty());
      }
    }
  }

  GIVEN("a site that can build nothing") {
    auto site = world.entity();

    WHEN("its palette is built") {
      THEN("it is empty") { CHECK(buildPalette(site).empty()); }
    }
  }
}
