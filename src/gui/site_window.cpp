#include "site_window.h"
#include "components.h"
#include <imgui.h>

void SiteWindow::draw(flecs::world &world) {
  if (!visible)
    return;

  auto entity = world.lookup("cape_canaveral");
  auto site = entity.get<Site>();
  auto numBuildings = 0;
  entity.children([&](flecs::entity) { numBuildings++; });

  ImGui::Begin("Site");
  ImGui::Text("Name: %s", site->name.c_str());
  ImGui::Text("Buildings: %d", numBuildings);
  ImGui::End();
}
