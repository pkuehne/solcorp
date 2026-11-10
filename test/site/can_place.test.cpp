#include "modules/site/placement.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string>
#include <vector>

namespace {

OccupancyGrid gridFrom(const std::vector<std::string> &rows) {
  OccupancyGrid grid{.width = static_cast<uint8_t>(rows.front().size()),
                     .height = static_cast<uint8_t>(rows.size()),
                     .cells = {}};
  for (const auto &row : rows) {
    for (char c : row) {
      grid.cells.push_back(c == 'B'   ? CellContent::Building
                           : c == 'C' ? CellContent::Connector
                                      : CellContent::Empty);
    }
  }
  return grid;
}

// A road along row 1, a building at (4, 3)
const std::vector<std::string> SITE = {
    ".....",
    "CCCCC",
    ".....",
    "....B",
};

const std::vector<std::string> EMPTY_SITE = {
    ".....",
    ".....",
    ".....",
};

} // namespace

SCENARIO("canPlace for buildings", "[site][placement]") {
  struct Case {
    std::vector<std::string> rows;
    SiteLocation origin;
    Footprint footprint;
    PlacementResult expected;
  };

  GIVEN("a site and a building footprint") {
    auto [rows, origin, footprint, expected] = GENERATE(values<Case>({
        // below the road
        {SITE, {.x = 0, .y = 2}, {.w = 2, .h = 2}, PlacementResult::Ok},
        // above the road
        {SITE, {.x = 2, .y = 0}, {.w = 1, .h = 1}, PlacementResult::Ok},
        // overlaps the road
        {SITE, {.x = 0, .y = 0}, {.w = 2, .h = 2}, PlacementResult::Occupied},
        // overlaps the building
        {SITE, {.x = 3, .y = 2}, {.w = 2, .h = 2}, PlacementResult::Occupied},
        // runs off the right edge
        {SITE,
         {.x = 4, .y = 2},
         {.w = 2, .h = 1},
         PlacementResult::OutOfBounds},
        // runs off the bottom edge
        {SITE,
         {.x = 0, .y = 3},
         {.w = 1, .h = 2},
         PlacementResult::OutOfBounds},
        // only next to another building
        {SITE,
         {.x = 3, .y = 3},
         {.w = 1, .h = 1},
         PlacementResult::NotConnectedToRoad},
        // one row away from the road
        {SITE,
         {.x = 0, .y = 3},
         {.w = 2, .h = 1},
         PlacementResult::NotConnectedToRoad},
        // an empty site needs a road first
        {EMPTY_SITE,
         {.x = 1, .y = 1},
         {.w = 2, .h = 2},
         PlacementResult::NotConnectedToRoad},
    }));
    CAPTURE(origin.x, origin.y, footprint.w, footprint.h);

    THEN("placing it gives the expected result") {
      CHECK(canPlace(gridFrom(rows), PlacementKind::Building, origin,
                     footprint) == expected);
    }
  }
}

SCENARIO("canPlace for connectors", "[site][placement]") {
  struct Case {
    std::vector<std::string> rows;
    SiteLocation origin;
    PlacementResult expected;
  };

  GIVEN("a site and a connector cell") {
    auto [rows, origin, expected] = GENERATE(values<Case>({
        // extends the road
        {SITE, {.x = 0, .y = 0}, PlacementResult::Ok},
        // next to the building only
        {SITE, {.x = 4, .y = 2}, PlacementResult::Ok},
        // on the road
        {SITE, {.x = 2, .y = 1}, PlacementResult::Occupied},
        // isolated
        {{".....", "C....", ".....", "....."},
         {.x = 3, .y = 3},
         PlacementResult::ConnectorNotConnected},
        // diagonal neighbours do not connect
        {{".....", ".C...", ".....", "....."},
         {.x = 2, .y = 2},
         PlacementResult::ConnectorNotConnected},
        // off the site
        {SITE, {.x = 5, .y = 0}, PlacementResult::OutOfBounds},
        // the first connector on an empty site goes anywhere
        {EMPTY_SITE, {.x = 4, .y = 2}, PlacementResult::Ok},
        // but still not off the site
        {EMPTY_SITE, {.x = 0, .y = 3}, PlacementResult::OutOfBounds},
    }));
    CAPTURE(origin.x, origin.y);

    THEN("placing it gives the expected result, ignoring any footprint") {
      CHECK(canPlace(gridFrom(rows), PlacementKind::Connector, origin,
                     Footprint{.w = 3, .h = 3}) == expected);
    }
  }
}
