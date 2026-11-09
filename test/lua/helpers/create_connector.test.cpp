#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/lua/helpers.h"
#include "modules/site/connector.h"
#include "setup_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_connector links its layers to tilesets", "[helpers][lua]") {
  flecs::world world;
  world.import <BaseModule>();
  registerEngineComponents(world);
  run_site_prefab_setup(world);
  auto site = world.entity("SiteA");
  auto tilesets = world.entity("Tilesets");
  auto asphalt = world.entity("asphalt").child_of(tilesets);
  auto markings = world.entity("markings").child_of(tilesets);

  GIVEN("base and markings tileset names that exist") {
    WHEN("create_connector is called") {
      auto road = create_connector(world, "Road", ConnectorVariant::Straight, 0,
                                   1, 2, site, "asphalt", "markings");

      THEN("each layer is linked to its tileset") {
        REQUIRE(road.is_valid());
        CHECK(road.target<TilesetBase>() == asphalt);
        CHECK(road.target<TilesetMarkings>() == markings);
      }
    }
  }

  GIVEN("no markings tileset") {
    WHEN("create_connector is called") {
      auto road = create_connector(world, "Road", ConnectorVariant::Straight, 0,
                                   1, 2, site, "asphalt", "");

      THEN("only the base layer is linked") {
        REQUIRE(road.is_valid());
        CHECK(road.target<TilesetBase>() == asphalt);
        CHECK_FALSE(road.target<TilesetMarkings>().is_valid());
      }
    }
  }

  GIVEN("a base tileset name that does not exist") {
    WHEN("create_connector is called") {
      auto road = create_connector(world, "Road", ConnectorVariant::Straight, 0,
                                   1, 2, site, "no_such_tileset", "");

      THEN("the connector is created without a base layer") {
        REQUIRE(road.is_valid());
        CHECK_FALSE(road.target<TilesetBase>().is_valid());
      }
    }
  }
}
