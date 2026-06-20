#include "modules/lua/effect_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

namespace {

using solcorp::test::TempLuaFile;

const EffectDef *findEffect(const std::vector<EffectDef> &effects,
                            const std::string &id) {
  auto it = std::ranges::find_if(
      effects, [&](const EffectDef &e) { return e.id == id; });
  return it == effects.end() ? nullptr : &*it;
}

} // namespace

SCENARIO("parseEffectData reads effect template definitions") {
  GIVEN("an effects.lua with an id-keyed map of effects") {
    TempLuaFile file(R"(
      return {
        better_concrete = {
          name = "Better Concrete",
          modifiers = {
            { target_stat = "max-weight", multiplicative = 1.2 },
          },
        },
        reinforcing_struts = {
          name = "Reinforcing Struts",
          modifiers = {
            { target_stat = "max-weight", additive = 500 },
            { target_stat = "prep-days", multiplicative = 0.9 },
          },
        },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<EffectDef> effects = parseEffectData(lua.root());

      THEN("every effect entry is captured keyed by its id") {
        REQUIRE(effects.size() == 2);
        REQUIRE(findEffect(effects, "better_concrete") != nullptr);
        REQUIRE(findEffect(effects, "reinforcing_struts") != nullptr);
      }

      THEN("an effect's display name is parsed") {
        const EffectDef *concrete = findEffect(effects, "better_concrete");
        REQUIRE(concrete != nullptr);
        CHECK(concrete->name == "Better Concrete");
      }

      THEN("an effect's modifiers are parsed with their stat and values") {
        const EffectDef *concrete = findEffect(effects, "better_concrete");
        REQUIRE(concrete != nullptr);
        REQUIRE(concrete->modifiers.size() == 1);
        CHECK(concrete->modifiers[0].target_stat == "max-weight");
        CHECK(concrete->modifiers[0].multiplicative == 1.2);
        // additive defaults to 0 when absent.
        CHECK(concrete->modifiers[0].additive == 0.0);
      }

      THEN("an effect can carry several modifiers") {
        const EffectDef *struts = findEffect(effects, "reinforcing_struts");
        REQUIRE(struts != nullptr);
        REQUIRE(struts->modifiers.size() == 2);
        // multiplicative defaults to 1 when absent.
        CHECK(struts->modifiers[0].target_stat == "max-weight");
        CHECK(struts->modifiers[0].additive == 500);
        CHECK(struts->modifiers[0].multiplicative == 1.0);
      }
    }
  }

  GIVEN("an effect entry without a name or modifiers") {
    TempLuaFile file(R"(
      return {
        placeholder = {},
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<EffectDef> effects = parseEffectData(lua.root());

      THEN("the name defaults to the id and no modifiers are produced") {
        const EffectDef *effect = findEffect(effects, "placeholder");
        REQUIRE(effect != nullptr);
        CHECK(effect->name == "placeholder");
        CHECK(effect->modifiers.empty());
      }
    }
  }

  GIVEN("an empty table") {
    TempLuaFile file("return {}");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<EffectDef> effects = parseEffectData(lua.root());

      THEN("no effects are produced") { REQUIRE(effects.empty()); }
    }
  }
}
