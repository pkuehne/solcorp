#include "modules/lua/logging.h"
#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("solcorp.logging.info writes to the mod logger") {
  GIVEN("a Lua state with load_logging configured") {
    spdlog::drop("logmod");
    lua_State *L = make_state_with_mod("logmod");
    load_logging(L);

    std::ostringstream oss;
    redirect_logger("logmod", oss);

    WHEN("solcorp.logging.info is called from Lua") {
      luaL_dostring(L, R"(solcorp.logging.info("hello info"))");
      spdlog::get("logmod")->flush();
      THEN("the message appears in the captured output") {
        REQUIRE(oss.str().find("hello info") != std::string::npos);
      }
    }

    spdlog::drop("logmod");
    lua_close(L);
  }
}

SCENARIO("solcorp.logging.warn writes to the mod logger") {
  GIVEN("a Lua state with load_logging configured") {
    spdlog::drop("warnmod");
    lua_State *L = make_state_with_mod("warnmod");
    load_logging(L);

    std::ostringstream oss;
    redirect_logger("warnmod", oss);

    WHEN("solcorp.logging.warn is called from Lua") {
      luaL_dostring(L, R"(solcorp.logging.warn("hello warn"))");
      spdlog::get("warnmod")->flush();
      THEN("the message appears in the captured output") {
        REQUIRE(oss.str().find("hello warn") != std::string::npos);
      }
    }

    spdlog::drop("warnmod");
    lua_close(L);
  }
}

SCENARIO("solcorp.logging.error writes to the mod logger") {
  GIVEN("a Lua state with load_logging configured") {
    spdlog::drop("errmod");
    lua_State *L = make_state_with_mod("errmod");
    load_logging(L);

    std::ostringstream oss;
    redirect_logger("errmod", oss);

    WHEN("solcorp.logging.error is called from Lua") {
      luaL_dostring(L, R"(solcorp.logging.error("hello error"))");
      spdlog::get("errmod")->flush();
      THEN("the message appears in the captured output") {
        REQUIRE(oss.str().find("hello error") != std::string::npos);
      }
    }

    spdlog::drop("errmod");
    lua_close(L);
  }
}
