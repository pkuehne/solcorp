
#include "config.h"
#include "lua.h"
#include "modules/base/base.h"
#include "spdlog/spdlog.h"

Config load_config_file() {
  Config config;

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  if (luaL_dofile(L, "config.lua") != LUA_OK) {
    spdlog::error("Failed to load config.lua: {}", lua_tostring(L, -1));
    lua_close(L);
    return config;
  }

  if (!lua_istable(L, -1)) {
    spdlog::error("config.lua must return a table");
    lua_close(L);
    return config;
  }

  lua_getfield(L, -1, "font");
  if (lua_isstring(L, -1)) {
    config.font = lua_tostring(L, -1);
  }
  lua_pop(L, 1);

  lua_getfield(L, -1, "font_size");
  if (lua_isnumber(L, -1)) {
    config.font_size = static_cast<uint32_t>(lua_tointeger(L, -1));
  }
  lua_pop(L, 1);

  spdlog::info("Config loaded: font='{}', font_size={}", config.font,
               config.font_size);

  lua_close(L);
  return config;
}