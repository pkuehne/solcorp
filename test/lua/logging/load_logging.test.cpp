#include "test_helpers.h"
#include "modules/lua/logging.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("load_logging registers a named spdlog logger") {
  GIVEN("a Lua state with solcorp table and mod name set") {
    spdlog::drop("mymod");
    lua_State *L = make_state_with_mod("mymod");

    WHEN("load_logging is called") {
      load_logging(L);
      THEN("a logger named after the mod is registered") {
        REQUIRE(spdlog::get("mymod") != nullptr);
      }
    }

    spdlog::drop("mymod");
    lua_close(L);
  }
}
