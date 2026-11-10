#include "modules/lua/helpers.h"
#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_site", "[helpers][lua]") {
  flecs::world world;
  world.import <SiteModule>();
  world.entity("Earth").child_of(world.entity("Sun"));

  GIVEN("a name and dimensions") {
    WHEN("created without make_current") {
      auto site = create_site(world, "Alpha", 10, 20);
      THEN("entity is valid with correct dimensions") {
        REQUIRE(site.is_valid());
        auto &s = site.get<Site>();
        CHECK(s.width == 10);
        CHECK(s.height == 20);
      }
      THEN("entity has SiteNeedsRelayout") {
        CHECK(site.has<SiteNeedsRelayout>());
      }
      THEN("entity does not have CurrentSite") {
        CHECK(!site.has<CurrentSite>());
      }
    }

    WHEN("created with make_current = true") {
      auto site = create_site(world, "Beta", 5, 5, true);
      THEN("entity has CurrentSite") { CHECK(site.has<CurrentSite>()); }
    }
  }

  GIVEN("buildable categories, one of which does not exist") {
    auto prefabs = world.entity("Prefabs");
    auto buildings = world.entity("Buildings").child_of(prefabs);
    auto modules = world.entity("Modules").child_of(prefabs);

    WHEN("the site is created with them") {
      auto site = create_site(world, "Gamma", 5, 5, false,
                              {"Buildings", "NoSuchCategory", "Modules"});

      THEN("the site is buildable for the categories that exist") {
        CHECK(site.has<Buildable>(buildings));
        CHECK(site.has<Buildable>(modules));
        int links = 0;
        site.each<Buildable>([&](flecs::entity) { ++links; });
        CHECK(links == 2);
      }
    }

    WHEN("the site is created without them") {
      auto site = create_site(world, "Delta", 5, 5);

      THEN("nothing is buildable on it") {
        CHECK_FALSE(site.has<Buildable>(flecs::Wildcard));
      }
    }
  }
}
