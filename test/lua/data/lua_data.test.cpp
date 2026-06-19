#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

// Writes Lua source to a uniquely-named temp file and removes it on scope exit,
// so each test exercises the real on-disk load path of LuaDataFile.
class TempLuaFile {
public:
  explicit TempLuaFile(const std::string &contents) {
    static int counter = 0;
    path_ = std::filesystem::temp_directory_path() /
            ("lua_data_test_" + std::to_string(counter++) + ".lua");
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

} // namespace

SCENARIO("LuaDataFile loads a file that returns a table") {
  GIVEN("a file that returns a table") {
    TempLuaFile file("return { name = 'core' }");

    WHEN("it is opened") {
      LuaDataFile data(file.path());

      THEN("it reports success and an empty error") {
        REQUIRE(data.ok());
        REQUIRE(data.error().empty());
      }
      THEN("the root table is readable") {
        REQUIRE(data.root().getString("name") == "core");
      }
    }
  }

  GIVEN("a path that does not exist") {
    LuaDataFile data("/no/such/file/definitely_missing.lua");

    THEN("it fails with a non-empty error") {
      REQUIRE_FALSE(data.ok());
      REQUIRE_FALSE(data.error().empty());
    }
  }

  GIVEN("a file that does not return a table") {
    TempLuaFile file("return 42");
    LuaDataFile data(file.path());

    THEN("it fails, naming the did-not-return-a-table reason") {
      REQUIRE_FALSE(data.ok());
      REQUIRE(data.error().find("did not return a table") != std::string::npos);
    }
  }

  GIVEN("a file with a syntax error") {
    TempLuaFile file("return { this is not valid lua");
    LuaDataFile data(file.path());

    THEN("it fails with a non-empty error") {
      REQUIRE_FALSE(data.ok());
      REQUIRE_FALSE(data.error().empty());
    }
  }
}

SCENARIO("LuaTableView reads typed scalar fields") {
  TempLuaFile file(R"(return {
    str = 'hello',
    int = 7,
    num = 3.5,
    flag = true,
  })");
  LuaDataFile data(file.path());
  REQUIRE(data.ok());
  LuaTableView root = data.root();

  GIVEN("fields of the matching type") {
    THEN("each typed getter returns the value") {
      REQUIRE(root.getString("str") == "hello");
      REQUIRE(root.getInt("int") == 7);
      REQUIRE(root.getNumber("num") == 3.5);
      REQUIRE(root.getBool("flag") == true);
    }
  }

  GIVEN("a missing key") {
    THEN("every getter returns nullopt") {
      REQUIRE_FALSE(root.getString("absent").has_value());
      REQUIRE_FALSE(root.getInt("absent").has_value());
      REQUIRE_FALSE(root.getNumber("absent").has_value());
      REQUIRE_FALSE(root.getBool("absent").has_value());
    }
  }

  GIVEN("a field of the wrong type") {
    THEN("type-mismatched getters return nullopt") {
      REQUIRE_FALSE(root.getInt("str").has_value());
      REQUIRE_FALSE(root.getString("int").has_value());
      REQUIRE_FALSE(root.getBool("int").has_value());
    }
  }
}

SCENARIO("LuaTableView iterates array elements") {
  GIVEN("an array of strings and nested tables") {
    TempLuaFile file(R"(return {
      items = {
        'first',
        { id = 'second' },
        'third',
      },
    })");
    LuaDataFile data(file.path());
    REQUIRE(data.ok());

    WHEN("forEachArrayElement walks the array") {
      std::vector<std::string> strings;
      std::vector<std::string> table_ids;
      data.root().forEachArrayElement("items", [&](const LuaValue &value) {
        if (auto s = value.asString()) {
          strings.push_back(*s);
        } else if (auto t = value.asTable()) {
          table_ids.push_back(t->getString("id").value_or(""));
        }
      });

      THEN("string elements are seen in order") {
        REQUIRE(strings == std::vector<std::string>{"first", "third"});
      }
      THEN("nested-table elements are seen as tables") {
        REQUIRE(table_ids == std::vector<std::string>{"second"});
      }
    }
  }

  GIVEN("a missing or non-table field") {
    TempLuaFile file("return { items = 5 }");
    LuaDataFile data(file.path());
    REQUIRE(data.ok());

    WHEN("forEachArrayElement is called") {
      int calls = 0;
      data.root().forEachArrayElement("items",
                                      [&](const LuaValue &) { ++calls; });
      data.root().forEachArrayElement("absent",
                                      [&](const LuaValue &) { ++calls; });

      THEN("the callback is never invoked") { REQUIRE(calls == 0); }
    }
  }
}

SCENARIO("LuaTableView iterates its own string-keyed entries") {
  GIVEN("a table mixing string keys, an array part, and nested tables") {
    TempLuaFile file(R"(return {
      alpha = { file = 'a.png' },
      beta = 'plain',
      'array_one',
      'array_two',
    })");
    LuaDataFile data(file.path());
    REQUIRE(data.ok());

    WHEN("forEachEntry walks the table") {
      std::vector<std::string> keys;
      std::vector<std::string> nested_files;
      data.root().forEachEntry(
          [&](const std::string &key, const LuaValue &value) {
            keys.push_back(key);
            if (auto t = value.asTable()) {
              nested_files.push_back(t->getString("file").value_or(""));
            }
          });

      THEN("only string keys are visited; array indices are skipped") {
        std::ranges::sort(keys);
        REQUIRE(keys == std::vector<std::string>{"alpha", "beta"});
      }
      THEN("entry values are accessible as tables") {
        REQUIRE(nested_files == std::vector<std::string>{"a.png"});
      }
    }
  }

  GIVEN("a table with no string keys") {
    TempLuaFile file("return { 'only', 'array', 'parts' }");
    LuaDataFile data(file.path());
    REQUIRE(data.ok());

    WHEN("forEachEntry is called") {
      int calls = 0;
      data.root().forEachEntry(
          [&](const std::string &, const LuaValue &) { ++calls; });

      THEN("the callback is never invoked") { REQUIRE(calls == 0); }
    }
  }
}

SCENARIO("LuaTableView reads a nested table field with withTable") {
  GIVEN("a field that is a table") {
    TempLuaFile file(R"(return {
      sprite = { texture = 'Buildings', x = 3 },
    })");
    LuaDataFile data(file.path());
    REQUIRE(data.ok());

    WHEN("withTable is invoked for that field") {
      bool called = false;
      std::string texture;
      long long x = 0;
      data.root().withTable("sprite", [&](const LuaTableView &sprite) {
        called = true;
        texture = sprite.getString("texture").value_or("");
        x = sprite.getInt("x").value_or(0);
      });

      THEN("the callback runs with a readable view of the nested table") {
        REQUIRE(called);
        REQUIRE(texture == "Buildings");
        REQUIRE(x == 3);
      }
    }

    WHEN("withTable is used inside forEachArrayElement / forEachEntry") {
      // Regression: withTable must leave the Lua stack balanced so an enclosing
      // iteration is not corrupted.
      int entries = 0;
      data.root().forEachEntry([&](const std::string &, const LuaValue &v) {
        ++entries;
        if (auto t = v.asTable()) {
          t->withTable("missing", [](const LuaTableView &) {});
        }
      });

      THEN("the enclosing iteration completes normally") {
        REQUIRE(entries == 1);
      }
    }
  }

  GIVEN("a field that is absent or not a table") {
    TempLuaFile file("return { sprite = 5 }");
    LuaDataFile data(file.path());
    REQUIRE(data.ok());

    WHEN("withTable is called") {
      int calls = 0;
      data.root().withTable("sprite", [&](const LuaTableView &) { ++calls; });
      data.root().withTable("absent", [&](const LuaTableView &) { ++calls; });

      THEN("the callback is never invoked") { REQUIRE(calls == 0); }
    }
  }
}
