#pragma once

#include "modules/stats/stats.h"
#include <format>
#include <functional>
#include <string>

namespace Widgets {
void StatTooltip(
    const Stat *stat, const std::function<std::string(double)> &fmt =
                          [](double v) { return std::format("{:.0f}", v); });
} // namespace Widgets
