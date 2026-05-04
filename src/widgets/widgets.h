#pragma once

#include <string>

struct ButtonLabel {
  const char *text;
};

struct ButtonTooltip {
  const char *text;
};

bool ActionButton(ButtonLabel, ButtonTooltip, const std::string &issue);
void NotImplementedPopup();
