#pragma once

#include <string>

struct ButtonLabel {
  const char *text;
};

struct ButtonTooltip {
  const char *text;
};

namespace Widgets {
bool ActionButton(ButtonLabel, ButtonTooltip, const std::string &issue);
bool SmallActionButton(ButtonLabel, ButtonTooltip, const std::string &issue);
void NotImplementedPopup();
} // namespace Widgets
