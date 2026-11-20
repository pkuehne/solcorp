#pragma once
#include <flecs.h>
#include <vector>

struct SiteWindow {};

struct PaletteSection {
  flecs::entity category;
  std::vector<flecs::entity> prefabs;
};

/// @brief One section per Buildable target, prefabs in creation order
std::vector<PaletteSection> buildPalette(flecs::entity site);

void showSiteWindow(flecs::world &world);
void drawSiteWindow(flecs::entity winE);
