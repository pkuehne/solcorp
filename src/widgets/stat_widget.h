#pragma once

#include "modules/stats/stats.h"
#include <string>

namespace Widgets {
struct ModifierDisplayOptions {
  Stat::Format fallback_format = Stat::Format::Number;
  bool fallback_higher_is_better = true;
};

[[nodiscard]] std::string ModifierValueText(
    const Modifier &mod, const Stat *stat = nullptr,
    ModifierDisplayOptions options = {});

/// Draws a single labeled modifier line. Returns false when there is no visible
/// modifier value to display.
bool ModifierLine(const char *label, const Modifier &mod,
                  const Stat *stat = nullptr,
                  ModifierDisplayOptions options = {});

void StatTooltip(const Stat *stat);
} // namespace Widgets
