#include "modules/lua/texture_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

namespace {

using solcorp::test::TempLuaFile;

const TextureDef *findTexture(const std::vector<TextureDef> &textures,
                              const std::string &name) {
  auto it = std::ranges::find_if(
      textures, [&](const TextureDef &t) { return t.name == name; });
  return it == textures.end() ? nullptr : &*it;
}

} // namespace

SCENARIO("parseTextureData reads an id-keyed map of texture definitions") {
  GIVEN("a textures.lua with table-valued entries") {
    TempLuaFile file(R"(
      return {
        Buildings = { file = "buildings.png" },
        Terrain   = { file = "terrain.png" },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<TextureDef> textures = parseTextureData(lua.materialize());

      THEN("each entry is captured keyed by its name with its file") {
        REQUIRE(textures.size() == 2);
        const TextureDef *buildings = findTexture(textures, "Buildings");
        REQUIRE(buildings != nullptr);
        REQUIRE(buildings->file == "buildings.png");
        const TextureDef *terrain = findTexture(textures, "Terrain");
        REQUIRE(terrain != nullptr);
        REQUIRE(terrain->file == "terrain.png");
      }
    }
  }

  GIVEN("an empty table") {
    TempLuaFile file("return {}");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<TextureDef> textures = parseTextureData(lua.materialize());

      THEN("no textures are produced") { REQUIRE(textures.empty()); }
    }
  }
}
