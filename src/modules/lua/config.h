#pragma once

#include "modules/base/base.h"

struct lua_State;

/// @brief Parse a Config from the table at the top of the Lua stack.
/// @return Config with values from the table, defaults for missing/wrong-typed
/// fields.
Config parse_config(lua_State *L);

/// @brief Load config.lua and apply global startup settings.
/// @return Config struct with loaded settings, or defaults on failure.
Config load_config_file();
