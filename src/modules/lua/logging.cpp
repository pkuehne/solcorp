#include "logging.h"
#include "lua_registry.h"
#include <spdlog/spdlog.h>

static int log_info(lua_State *L) {
  const char *msg = luaL_checkstring(L, 1);
  std::string mod_name = lua_get_mod_name(L);
  auto logger = spdlog::get(mod_name);
  if (logger) {
    logger->info("{}", msg);
  }
  return 0;
}

static int log_warn(lua_State *L) {
  const char *msg = luaL_checkstring(L, 1);
  std::string mod_name = lua_get_mod_name(L);
  auto logger = spdlog::get(mod_name);
  if (logger) {
    logger->warn("{}", msg);
  }
  return 0;
}

static int log_error(lua_State *L) {
  const char *msg = luaL_checkstring(L, 1);
  std::string mod_name = lua_get_mod_name(L);
  auto logger = spdlog::get(mod_name);
  if (logger) {
    logger->error("{}", msg);
  }
  return 0;
}

void load_logging(lua_State *L) {
  std::string mod_name = lua_get_mod_name(L);
  auto sink = spdlog::default_logger()->sinks()[0];
  auto logger = std::make_shared<spdlog::logger>(mod_name, sink);
  spdlog::register_logger(logger);

  lua_getglobal(L, "solcorp");
  lua_get_or_create_table(L, "logging");

  lua_register_function(L, "info", log_info);
  lua_register_function(L, "warn", log_warn);
  lua_register_function(L, "error", log_error);

  lua_pop(L, 2);
}
