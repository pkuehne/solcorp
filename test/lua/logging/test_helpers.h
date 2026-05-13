#pragma once
#include "modules/lua/lua_registry.h"
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

static lua_State *make_state_with_mod(const std::string &mod_name) {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  lua_newtable(L);
  lua_setglobal(L, "solcorp");
  lua_set_mod_name(L, mod_name);
  return L;
}

static std::shared_ptr<spdlog::sinks::ostream_sink_st>
redirect_logger(const std::string &name, std::ostringstream &oss) {
  auto logger = spdlog::get(name);
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(oss);
  logger->sinks() = {sink};
  logger->set_level(spdlog::level::trace);
  return sink;
}
