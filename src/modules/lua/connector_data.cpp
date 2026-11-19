#include "modules/lua/connector_data.h"

std::vector<ConnectorDef> parseConnectorData(const ModValue &root) {
  std::vector<ConnectorDef> connectors;
  root.forEachEntry([&](const std::string &id, const ModValue &value) {
    if (value.isTable()) {
      connectors.push_back(
          {.id = id,
           .name = value.getString("name").value_or(id),
           .base = value.getString("base").value_or(""),
           .markings = value.getString("markings").value_or("")});
    }
  });
  return connectors;
}
