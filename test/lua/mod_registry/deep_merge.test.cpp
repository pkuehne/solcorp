#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include "modules/lua/mod_registry.h"
#include "modules/lua/mod_value.h"
#include <catch2/catch_test_macros.hpp>

namespace {

using solcorp::test::TempLuaFile;

// Materialise an id-keyed mod table from Lua source, as a mod's data file would
// be read before folding into the registry.
ModValue table(const std::string &lua) {
  TempLuaFile file(lua);
  LuaDataFile data(file.path());
  REQUIRE(data.ok());
  return data.materialize();
}

} // namespace

SCENARIO("ModRegistry deep-merges mod data tables in load order") {
  ModRegistry registry;

  GIVEN("a base mod and a later mod that patches one nested field") {
    registry.merge("buildings", table(R"(
      return {
        launch_complex = {
          name = "Launch Complex",
          sprite = { texture = "Buildings", x = 0, y = 96 },
        },
      }
    )"),
                   "core", 0);
    registry.merge("buildings", table(R"(
      return { launch_complex = { sprite = { x = 32 } } }
    )"),
                   "scenario", 1);

    WHEN("the merged entry is read back") {
      const ModValue &buildings = registry.merged("buildings");

      THEN("the patched nested field wins") {
        const ModValue &def = buildings.fields().at("launch_complex");
        bool checked = false;
        def.withTable("sprite", [&](const ModValue &sprite) {
          checked = true;
          REQUIRE(sprite.getInt("x") == 32);
        });
        REQUIRE(checked);
      }

      THEN("sibling fields of the patched map are preserved") {
        const ModValue &def = buildings.fields().at("launch_complex");
        REQUIRE(def.getString("name") == "Launch Complex");
        def.withTable("sprite", [&](const ModValue &sprite) {
          REQUIRE(sprite.getString("texture") == "Buildings");
          REQUIRE(sprite.getInt("y") == 96);
        });
      }
    }

    WHEN("provenance is inspected") {
      THEN("both mods that changed the entry are recorded in order") {
        REQUIRE(registry.historyString("buildings", "launch_complex") ==
                "core(0) -> scenario(1)");
      }
    }
  }

  GIVEN("a later mod that re-declares an identical value (a no-op)") {
    registry.merge("buildings", table(R"(
      return { office = { name = "Office" } }
    )"),
                   "core", 0);
    registry.merge("buildings", table(R"(
      return { office = { name = "Office" } }
    )"),
                   "cosmetic", 1);

    THEN("provenance records only the mod that actually changed the entry") {
      REQUIRE(registry.historyString("buildings", "office") == "core(0)");
    }
  }

  GIVEN("a base list field and a later mod that overrides it") {
    registry.merge("buildings", table(R"(
      return {
        factory = {
          facilities = {
            { name = "Line 1", type = "Manufacturing" },
            { name = "Line 2", type = "Manufacturing" },
          },
        },
      }
    )"),
                   "core", 0);
    registry.merge("buildings", table(R"(
      return { factory = { facilities = { { name = "Only", type = "Office" } } } }
    )"),
                   "overhaul", 1);

    THEN("the list is replaced wholesale, not merged element-wise") {
      const ModValue &def = registry.merged("buildings").fields().at("factory");
      std::vector<std::string> names;
      def.forEachArrayElement("facilities", [&](const ModValue &f) {
        names.push_back(f.getString("name").value_or(""));
      });
      REQUIRE(names == std::vector<std::string>{"Only"});
    }
  }

  GIVEN("a later mod that DELETEs a single field") {
    registry.merge("buildings", table(R"(
      return { silo = { name = "Silo", sprite = { texture = "T" } } }
    )"),
                   "core", 0);
    registry.merge("buildings", table(R"(
      return { silo = { sprite = "__DELETE__" } }
    )"),
                   "scenario", 1);

    THEN("only the deleted field is removed; the entry survives") {
      const ModValue &def = registry.merged("buildings").fields().at("silo");
      REQUIRE(def.getString("name") == "Silo");
      bool has_sprite = false;
      def.withTable("sprite", [&](const ModValue &) { has_sprite = true; });
      REQUIRE_FALSE(has_sprite);
    }
  }

  GIVEN("a later mod that DELETEs a whole entry") {
    registry.merge("buildings", table(R"(
      return { legacy = { name = "Legacy" }, keep = { name = "Keep" } }
    )"),
                   "core", 0);
    registry.merge("buildings", table(R"(
      return { legacy = "__DELETE__" }
    )"),
                   "overhaul", 1);

    THEN("the deleted entry is gone and others remain") {
      const ModValue &buildings = registry.merged("buildings");
      REQUIRE_FALSE(buildings.fields().contains("legacy"));
      REQUIRE(buildings.fields().contains("keep"));
    }
  }

  GIVEN("per-animation patching across mods") {
    registry.merge("buildings", table(R"(
      return {
        gantry = {
          animations = {
            idle = { frames = {1, 2, 3}, fps = 4, loop = true },
          },
        },
      }
    )"),
                   "core", 0);
    registry.merge("buildings", table(R"(
      return { gantry = { animations = { idle = { fps = 8 } } } }
    )"),
                   "cosmetic", 1);

    THEN("the single patched animation parameter wins, others are kept") {
      const ModValue &def = registry.merged("buildings").fields().at("gantry");
      def.withTable("animations", [&](const ModValue &anims) {
        anims.withTable("idle", [&](const ModValue &idle) {
          REQUIRE(idle.getInt("fps") == 8);
          REQUIRE(idle.getBool("loop") == true);
          std::vector<long long> frames;
          idle.forEachArrayElement("frames", [&](const ModValue &f) {
            frames.push_back(f.asInt().value_or(-1));
          });
          REQUIRE(frames == std::vector<long long>{1, 2, 3});
        });
      });
    }
  }

  GIVEN("an unknown category or entry") {
    THEN("merged() is an empty table and historyString() is empty") {
      REQUIRE(registry.merged("missing").fields().empty());
      REQUIRE(registry.historyString("missing", "x").empty());
    }
  }
}
