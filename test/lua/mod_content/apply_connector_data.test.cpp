#include "../helpers/setup_helpers.h"
#include "modules/base/base.h"
#include "modules/engine/render.h"
#include "modules/lua/mod_content.h"
#include "modules/site/connector.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("applyConnectorData creates connector prefabs",
         "[connector][mod_content][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);
  auto tilesets = world.entity("Tilesets");
  auto asphalt = world.entity("asphalt").child_of(tilesets).set<Tileset>({});
  auto lines = world.entity("lines").child_of(tilesets).set<Tileset>({});

  GIVEN("a connector with both tilesets") {
    ConnectorDef road{
        .id = "road", .name = "Road", .base = "asphalt", .markings = "lines"};

    WHEN("it is applied") {
      applyConnectorData(world, {road});
      auto prefab = world.lookup("Prefabs::Connectors::road");

      THEN("a labelled connector prefab points at both tilesets") {
        REQUIRE(prefab.is_valid());
        CHECK(prefab.has(flecs::Prefab));
        CHECK(prefab.get<Label>().label == "Road");
        CHECK(prefab.has<ConnectorTile>());
        CHECK(prefab.target<TilesetBase>() == asphalt);
        CHECK(prefab.target<TilesetMarkings>() == lines);
      }

      THEN("an instance of it sees the same tilesets") {
        REQUIRE(prefab.is_valid());
        auto instance = world.entity().is_a(prefab);
        CHECK(instance.has<ConnectorTile>());
        CHECK(instance.target<TilesetBase>() == asphalt);
        CHECK(instance.target<TilesetMarkings>() == lines);
      }

      AND_WHEN("it is reapplied without markings") {
        road.markings = "";
        applyConnectorData(world, {road});

        THEN("the markings are removed") {
          CHECK_FALSE(prefab.has<TilesetMarkings>(flecs::Wildcard));
          CHECK(prefab.target<TilesetBase>() == asphalt);
        }
      }
    }
  }

  GIVEN("a connector whose base tileset does not exist") {
    ConnectorDef road{
        .id = "road", .name = "Road", .base = "gravel", .markings = ""};

    WHEN("it is applied") {
      applyConnectorData(world, {road});

      THEN("no prefab is created") {
        CHECK_FALSE(world.lookup("Prefabs::Connectors::road").is_valid());
      }
    }
  }

  GIVEN("a connector whose markings tileset does not exist") {
    ConnectorDef road{
        .id = "road", .name = "Road", .base = "asphalt", .markings = "paint"};

    WHEN("it is applied") {
      applyConnectorData(world, {road});

      THEN("it is created without markings") {
        auto prefab = world.lookup("Prefabs::Connectors::road");
        REQUIRE(prefab.is_valid());
        CHECK(prefab.target<TilesetBase>() == asphalt);
        CHECK_FALSE(prefab.has<TilesetMarkings>(flecs::Wildcard));
      }
    }
  }
}
