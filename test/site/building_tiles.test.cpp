#include "modules/site/building_tiles.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <vector>

namespace {

std::vector<std::string> ids(const std::vector<const BuildingTile *> &tiles) {
  std::vector<std::string> result;
  result.reserve(tiles.size());
  for (const BuildingTile *tile : tiles) {
    result.push_back(tile->id);
  }
  return result;
}

} // namespace

SCENARIO("eligibleTiles finds the tiles that may fill a role", "[tile][site]") {
  const BuildingTiles tiles{
      .tiles = {
          {.id = "corner", .roles = {TileRole::TopLeft, TileRole::TopRight}},
          {.id = "roof_a", .roles = {TileRole::Centre}},
          {.id = "roof_b", .roles = {TileRole::Centre}},
          {.id = "launchpad",
           .roles = {TileRole::Centre},
           .predicate = "rollout_complete"},
          {.id = "glow", .roles = {TileRole::Centre}, .predicate = "powered"},
      }};

  struct Case {
    std::string description;
    TileRole role;
    std::vector<std::string> flags;
    std::vector<std::string> expected;
  };

  GIVEN("a building's tiles") {
    auto [description, role, flags, expected] = GENERATE(values<Case>({
        {"no flags set", TileRole::Centre, {}, {"roof_a", "roof_b"}},
        {"one flag set",
         TileRole::Centre,
         {"powered"},
         {"roof_a", "roof_b", "glow"}},
        {"both flags set",
         TileRole::Centre,
         {"rollout_complete", "powered"},
         {"roof_a", "roof_b", "launchpad", "glow"}},
        {"an unrelated flag",
         TileRole::Centre,
         {"damaged"},
         {"roof_a", "roof_b"}},
        {"a tile filling several roles", TileRole::TopRight, {}, {"corner"}},
        {"a role nothing fills", TileRole::Bottom, {"powered"}, {}},
    }));
    CAPTURE(description);

    WHEN("the eligible tiles for the role are requested") {
      auto eligible = eligibleTiles(tiles, role, flags);

      THEN("exactly the matching tiles are returned in registry order") {
        CHECK(ids(eligible) == expected);
      }
    }
  }
}

SCENARIO("pickTile chooses among candidates by weight", "[tile][site]") {
  const BuildingTile light{.id = "light", .weight = 1.0};
  const BuildingTile heavy{.id = "heavy", .weight = 3.0};
  const std::vector<const BuildingTile *> candidates = {&light, &heavy};

  struct Case {
    double roll;
    std::string expected;
  };

  GIVEN("a light tile with a quarter of the weight and a heavy one") {
    auto [roll, expected] = GENERATE(values<Case>({
        {0.0, "light"},
        {0.24, "light"},
        {0.25, "heavy"},
        {0.9, "heavy"},
        {0.9999999999, "heavy"},
    }));
    CAPTURE(roll);

    WHEN("a tile is picked") {
      const BuildingTile *picked = pickTile(candidates, roll);

      THEN("the roll falls in that tile's share of the weight") {
        REQUIRE(picked != nullptr);
        CHECK(picked->id == expected);
      }
    }
  }

  GIVEN("a single candidate") {
    WHEN("a tile is picked with any roll") {
      auto roll = GENERATE(0.0, 0.5, 0.99);
      CAPTURE(roll);

      THEN("that candidate is chosen") {
        CHECK(pickTile({&heavy}, roll) == &heavy);
      }
    }
  }

  GIVEN("no candidates") {
    WHEN("a tile is picked") {
      THEN("nothing is chosen") { CHECK(pickTile({}, 0.5) == nullptr); }
    }
  }
}

SCENARIO("tileRoleAt maps a footprint cell to its 9-slice role",
         "[tile][site]") {
  struct Case {
    int x;
    int y;
    int w;
    int h;
    TileRole expected;
  };

  GIVEN("a cell in a footprint") {
    auto [x, y, w, h, expected] = GENERATE(values<Case>({
        {0, 0, 4, 3, TileRole::TopLeft},
        {1, 0, 4, 3, TileRole::Top},
        {2, 0, 4, 3, TileRole::Top},
        {3, 0, 4, 3, TileRole::TopRight},
        {0, 1, 4, 3, TileRole::Left},
        {1, 1, 4, 3, TileRole::Centre},
        {3, 1, 4, 3, TileRole::Right},
        {0, 2, 4, 3, TileRole::BottomLeft},
        {2, 2, 4, 3, TileRole::Bottom},
        {3, 2, 4, 3, TileRole::BottomRight},
        {1, 1, 2, 2, TileRole::BottomRight},
        {0, 1, 2, 2, TileRole::BottomLeft},
    }));
    CAPTURE(x, y, w, h);

    THEN("its role follows from its row and column band") {
      CHECK(tileRoleAt(x, y, w, h) == expected);
    }
  }
}
