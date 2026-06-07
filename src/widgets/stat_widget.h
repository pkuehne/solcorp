#pragma once

#include "modules/stats/stats.h"
#include <string>

namespace Widgets {

[[nodiscard]] std::string ModifierValueText(const Modifier &mod,
                                            const StatDef &definition);

/// Draws a single labeled modifier line. Returns false when there is no visible
/// modifier value to display.
bool ModifierLine(const char *label, const Modifier &mod,
                  const StatDef &definition);

void StatTooltip(flecs::world &world, const Stat *stat);
} // namespace Widgets
