#include "modules/lua/config.h"
#include <catch2/catch_test_macros.hpp>
#include <lua.hpp>

static lua_State *make_state() {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  return L;
}

static void push_config_table(lua_State *L, const char *font = nullptr,
                              lua_Integer font_size = -1) {
  lua_newtable(L);
  if (font) {
    lua_pushstring(L, font);
    lua_setfield(L, -2, "font");
  }
  if (font_size >= 0) {
    lua_pushinteger(L, font_size);
    lua_setfield(L, -2, "font_size");
  }
}

SCENARIO("parse_config with no table on stack") {
  GIVEN("a Lua state with nil on top") {
    lua_State *L = make_state();
    lua_pushnil(L);

    WHEN("parse_config is called") {
      Config config = parse_config(L);

      THEN("default font is returned") {
        CHECK(config.font == "Roboto-Medium.ttf");
      }
      THEN("default font_size is returned") { CHECK(config.font_size == 16); }
    }

    lua_close(L);
  }
}

SCENARIO("parse_config with empty table") {
  GIVEN("a Lua state with an empty table on top") {
    lua_State *L = make_state();
    lua_newtable(L);

    WHEN("parse_config is called") {
      Config config = parse_config(L);

      THEN("default font is returned") {
        CHECK(config.font == "Roboto-Medium.ttf");
      }
      THEN("default font_size is returned") { CHECK(config.font_size == 16); }
    }

    lua_close(L);
  }
}

SCENARIO("parse_config with font field") {
  GIVEN("a config table with a font string") {
    lua_State *L = make_state();
    push_config_table(L, "MyFont.ttf");

    WHEN("parse_config is called") {
      Config config = parse_config(L);

      THEN("font is set from the table") { CHECK(config.font == "MyFont.ttf"); }
      THEN("font_size remains the default") { CHECK(config.font_size == 16); }
    }

    lua_close(L);
  }
}

SCENARIO("parse_config with font_size field") {
  GIVEN("a config table with a font_size number") {
    lua_State *L = make_state();
    push_config_table(L, nullptr, 24);

    WHEN("parse_config is called") {
      Config config = parse_config(L);

      THEN("font_size is set from the table") { CHECK(config.font_size == 24); }
      THEN("font remains the default") {
        CHECK(config.font == "Roboto-Medium.ttf");
      }
    }

    lua_close(L);
  }
}

SCENARIO("parse_config with both fields") {
  GIVEN("a config table with font and font_size") {
    lua_State *L = make_state();
    push_config_table(L, "Custom.ttf", 32);

    WHEN("parse_config is called") {
      Config config = parse_config(L);

      THEN("font is set from the table") { CHECK(config.font == "Custom.ttf"); }
      THEN("font_size is set from the table") { CHECK(config.font_size == 32); }
    }

    lua_close(L);
  }
}

SCENARIO("parse_config with wrong-typed fields") {
  GIVEN("a config table with a number for font and a string for font_size") {
    lua_State *L = make_state();
    lua_newtable(L);
    lua_pushinteger(L, 42);
    lua_setfield(L, -2, "font");
    lua_pushstring(L, "large");
    lua_setfield(L, -2, "font_size");

    WHEN("parse_config is called") {
      Config config = parse_config(L);

      THEN("font falls back to default") {
        CHECK(config.font == "Roboto-Medium.ttf");
      }
      THEN("font_size falls back to default") { CHECK(config.font_size == 16); }
    }

    lua_close(L);
  }
}
