#include "site_window.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/site/build_mode.h"
#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <flecs.h>
#include <string>

namespace {

const char *displayName(flecs::entity entity) {
  return entity.has<Label>() ? entity.get<Label>().label.c_str()
                             : entity.name().c_str();
}

} // namespace

std::vector<PaletteSection> buildPalette(flecs::entity site) {
  std::vector<PaletteSection> palette;
  site.each<Buildable>([&](flecs::entity category) {
    PaletteSection section{.category = category, .prefabs = {}};
    category.children(
        [&](flecs::entity prefab) { section.prefabs.push_back(prefab); });
    palette.push_back(section);
  });
  return palette;
}

void showSiteWindow(flecs::world &world) { showWindow(world, "Site Window"); }

void drawSiteWindow(flecs::entity winE) {
  auto world = winE.world();
  auto site =
      world.query_builder<const Site>().with<CurrentSite>().build().first();
  if (!site.is_valid()) {
    ImGui::TextUnformatted("No current site");
    return;
  }

  const auto &size = site.get<Site>();
  ImGui::Text("%s", site.name().c_str());
  ImGui::Text("Size: %d x %d", size.width, size.height);

  auto selected = buildModePrefab(world);
  for (const auto &section : buildPalette(site)) {
    ImGui::SeparatorText(displayName(section.category));
    for (auto prefab : section.prefabs) {
      ImGui::PushID(static_cast<int>(prefab.id()));
      if (ImGui::Selectable(displayName(prefab), prefab == selected)) {
        if (prefab == selected) {
          exitBuildMode(world);
        } else {
          enterBuildMode(world, prefab);
        }
      }
      ImGui::PopID();
    }
  }
}
