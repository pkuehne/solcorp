#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include "modules/lua/mod_value.h"
#include <catch2/catch_test_macros.hpp>

namespace {

using solcorp::test::TempLuaFile;

ModValue materialize(const std::string &lua) {
  TempLuaFile file(lua);
  LuaDataFile data(file.path());
  REQUIRE(data.ok());
  return data.materialize();
}

} // namespace

SCENARIO(
    "LuaDataFile::materialize turns a returned table into a ModValue tree") {
  GIVEN("a table of scalar fields of each type") {
    ModValue root = materialize(R"(
      return { name = "Base", count = 3, ratio = 1.5, active = true }
    )");

    THEN("each scalar is read back with its type") {
      REQUIRE(root.isTable());
      REQUIRE(root.getString("name") == "Base");
      REQUIRE(root.getInt("count") == 3);
      REQUIRE(root.getNumber("ratio") == 1.5);
      REQUIRE(root.getBool("active") == true);
    }

    THEN("a field of the wrong type reads as nullopt") {
      REQUIRE_FALSE(root.getInt("name").has_value());
      REQUIRE_FALSE(root.getString("count").has_value());
    }

    THEN("an absent field reads as nullopt") {
      REQUIRE_FALSE(root.getString("missing").has_value());
    }
  }

  GIVEN("a nested map field") {
    ModValue root = materialize(R"(
      return { sprite = { texture = "T", x = 4, y = 8 } }
    )");

    THEN("the nested map is materialised and reachable") {
      bool visited = false;
      root.withTable("sprite", [&](const ModValue &sprite) {
        visited = true;
        REQUIRE(sprite.getString("texture") == "T");
        REQUIRE(sprite.getInt("x") == 4);
      });
      REQUIRE(visited);
    }
  }

  GIVEN("an array-valued field") {
    ModValue root = materialize(R"(
      return {
        facilities = {
          { name = "A", type = "Office" },
          { name = "B", type = "Storage" },
        },
      }
    )");

    THEN("array elements are visited in order") {
      std::vector<std::string> names;
      root.forEachArrayElement("facilities", [&](const ModValue &f) {
        names.push_back(f.getString("name").value_or(""));
      });
      REQUIRE(names == std::vector<std::string>{"A", "B"});
    }

    THEN("the facilities value is a list, not a map") {
      const ModValue *facilities = nullptr;
      root.withTable("facilities", [&](const ModValue &f) { facilities = &f; });
      REQUIRE(facilities != nullptr);
      REQUIRE(facilities->isArrayLike());
    }
  }

  GIVEN("an empty table") {
    ModValue root = materialize("return {}");

    THEN("it is a table with no fields") {
      REQUIRE(root.isTable());
      REQUIRE(root.fields().empty());
      REQUIRE(root.array().empty());
    }
  }
}
