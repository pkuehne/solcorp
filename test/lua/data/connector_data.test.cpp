#include "modules/lua/connector_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <vector>

using solcorp::test::TempLuaFile;

SCENARIO("parseConnectorData reads connector types", "[connector][lua]") {
  struct Case {
    std::string entry;
    ConnectorDef expected;
  };

  GIVEN("a connectors.lua with one entry") {
    auto [entry, expected] = GENERATE(values<Case>({
        {R"({ name = "Road", base = "asphalt", markings = "lines" })",
         {.id = "road",
          .name = "Road",
          .base = "asphalt",
          .markings = "lines"}},
        {R"({ base = "asphalt" })",
         {.id = "road", .name = "road", .base = "asphalt", .markings = ""}},
        {R"({ name = "Road" })",
         {.id = "road", .name = "Road", .base = "", .markings = ""}},
    }));
    CAPTURE(entry);
    TempLuaFile file("return { road = " + entry + " }");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      auto connectors = parseConnectorData(lua.materialize());

      THEN("the entry is read with its defaults") {
        REQUIRE(connectors.size() == 1);
        CHECK(connectors[0].id == expected.id);
        CHECK(connectors[0].name == expected.name);
        CHECK(connectors[0].base == expected.base);
        CHECK(connectors[0].markings == expected.markings);
      }
    }
  }

  GIVEN("an entry that is not a table") {
    TempLuaFile file(R"(return { road = "asphalt" })");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    THEN("it is skipped") {
      CHECK(parseConnectorData(lua.materialize()).empty());
    }
  }
}
