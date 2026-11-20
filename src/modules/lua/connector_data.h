#pragma once

#include "modules/lua/mod_value.h"
#include <string>
#include <vector>

/// @brief A connector type (e.g. a road) a mod declares in connectors.lua
struct ConnectorDef {
  std::string id;
  std::string name;
  std::string base;     ///< Tileset drawn first
  std::string markings; ///< Tileset drawn over the base; empty for none
};

/// @brief Missing names default to the id; missing tilesets stay empty and
/// are rejected at apply time
std::vector<ConnectorDef> parseConnectorData(const ModValue &root);
