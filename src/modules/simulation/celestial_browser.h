#pragma once

#include <flecs.h>

struct CelestialBrowser {
  flecs::entity selected_body;
};

void showCelestialBrowser(flecs::world &world);
void drawCelestialBrowser(flecs::entity winE);
