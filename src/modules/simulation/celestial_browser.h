#pragma once

#include <flecs.h>

struct CelestialBrowser {
  flecs::entity selected_body;
};

void drawCelestialBrowser(flecs::entity winE);
