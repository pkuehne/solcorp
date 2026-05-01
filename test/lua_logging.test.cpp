#include "modules/lua/logging.h"
#include "modules/lua/lua_registry.h"
#include <catch2/catch_test_macros.hpp>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>

static lua_State *make_state_with_mod(const std::string &mod_name) {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  lua_newtable(L);
  lua_setglobal(L, "solcorp");
  lua_set_mod_name(L, mod_name);
  return L;
}

// Point an already-registered logger at a fresh ostream sink and return it.
// Does not touch the global default logger.
static std::shared_ptr<spdlog::sinks::ostream_sink_st>
redirect_logger(const std::string &name, std::ostringstream &oss) {
  auto logger = spdlog::get(name);
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(oss);
  logger->sinks() = {sink};
  logger->set_level(spdlog::level::trace);
  return sink;
}

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
