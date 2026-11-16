#pragma once
#include <flecs.h>
#include <vector>

struct SiteWindow {};

/// @brief One palette section: a Buildable category and the prefabs in it
struct PaletteSection {
  flecs::entity category;
  std::vector<flecs::entity> prefabs;
};

/// @brief The palette of @p site: one section per Buildable target, listing
/// that category's prefabs in creation order (ADR 009 §1)
std::vector<PaletteSection> buildPalette(flecs::entity site);

void showSiteWindow(flecs::world &world);
void drawSiteWindow(flecs::entity winE);
