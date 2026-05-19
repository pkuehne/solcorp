
#include "config.h"
#include "lua.h"
#include "modules/base/base.h"
#include "spdlog/spdlog.h"

#include <optional>
#include <type_traits>

template <typename T>
static std::optional<T> get_field(lua_State *L, const char *name) {
  lua_getfield(L, -1, name);
  std::optional<T> result;
  if constexpr (std::is_same_v<T, std::string>) {
    if (lua_type(L, -1) == LUA_TSTRING) {
      result = lua_tostring(L, -1);
    }
  } else if constexpr (std::is_integral_v<T>) {
    if (lua_isnumber(L, -1)) {
      result = static_cast<T>(lua_tointeger(L, -1));
    }
  } else if constexpr (std::is_floating_point_v<T>) {
    if (lua_isnumber(L, -1)) {
      result = static_cast<T>(lua_tonumber(L, -1));
    }
  }
  lua_pop(L, 1);
  return result;
}

Config parse_config(lua_State *L) {
  Config config;
  if (!lua_istable(L, -1)) {
    return config;
  }
  if (auto v = get_field<std::string>(L, "font")) {
    config.font = *v;
  }
  if (auto v = get_field<uint32_t>(L, "font_size")) {
    config.font_size = *v;
  }
  return config;
}

Config load_config_file() {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  if (luaL_dofile(L, "config.lua") != LUA_OK) {
    spdlog::error("Failed to load config.lua: {}", lua_tostring(L, -1));
    lua_close(L);
    return Config{};
  }

  Config config = parse_config(L);
  spdlog::info("Config loaded: font='{}', font_size={}", config.font,
               config.font_size);
  lua_close(L);
  return config;
}