#pragma once

#include <string>

struct ButtonLabel {
  const char *text;
};

struct ButtonTooltip {
  const char *text;
};

namespace Widgets {
bool ActionButton(ButtonLabel, ButtonTooltip, const std::string &issue,
                  bool small = false);
void NotImplementedPopup();
} // namespace Widgets
