#include "modules/lua/helpers.h"
#include "modules/site/site.h"
#include "setup_helpers.h"
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
      THEN("entity has ConstructionSiteNeedsUpdating") {
        CHECK(site.has<ConstructionSiteNeedsUpdating>());
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
}
