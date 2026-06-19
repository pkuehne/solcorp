#include "modules/lua/texture_data.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

// Writes Lua source to a uniquely-named temp file and removes it on scope exit,
// so each test exercises the real on-disk load path of LuaDataFile.
class TempLuaFile {
public:
  explicit TempLuaFile(const std::string &contents) {
    static int counter = 0;
    path_ = std::filesystem::temp_directory_path() /
            ("texture_data_test_" + std::to_string(counter++) + ".lua");
    std::ofstream(path_) << contents;
  }
  ~TempLuaFile() {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }

  TempLuaFile(const TempLuaFile &) = delete;
  TempLuaFile &operator=(const TempLuaFile &) = delete;

  [[nodiscard]] std::string path() const { return path_.string(); }

private:
  std::filesystem::path path_;
};

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
      std::vector<TextureDef> textures = parseTextureData(lua.root());

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
      std::vector<TextureDef> textures = parseTextureData(lua.root());

      THEN("no textures are produced") { REQUIRE(textures.empty()); }
    }
  }
}
