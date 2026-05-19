#pragma once

#include "modules/base/base.h"

/// @brief Load config.lua and apply global startup settings.
/// @return Config struct with loaded settings, or defaults on failure.
Config load_config_file();
