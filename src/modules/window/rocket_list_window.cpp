#include "rocket_list_window.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include "modules/rocket/rocket_module.h"
#include "modules/site/site.h"
#include "rocket_detail_window.h"
#include <flecs.h>

void showRocketListWindow(flecs::world &world) {
  showWindow(world, "Rocket List");
}

void drawRocketListWindow(flecs::entity winE) {
  auto world = winE.world();

  constexpr ImGuiTableFlags tableFlags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

  if (ImGui::BeginTable("rockets", 4, tableFlags)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 130.0f);
    ImGui::TableSetupColumn("Location");
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f);
    ImGui::TableHeadersRow();

    world.query_builder<Rocket>().build().each([&](flecs::entity rocketE,
                                                   Rocket &) {
      ImGui::TableNextRow();
      ImGui::PushID(std::to_string(rocketE.id()).c_str());

      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(rocketE.name().c_str());

      ImGui::TableSetColumnIndex(1);
      auto stateE = rocketE.target<RocketCurrentState>();
      ImGui::TextUnformatted(stateE.is_valid() ? stateE.name().c_str() : "-");

      ImGui::TableSetColumnIndex(2);
      auto facilityE = rocketE.parent();
      auto buildingE =
          facilityE.is_valid() ? facilityE.parent() : flecs::entity();
      if (buildingE.is_valid() && buildingE.has<Building>()) {
        ImGui::TextUnformatted(buildingE.name().c_str());
      } else if (facilityE.is_valid()) {
        ImGui::TextUnformatted(facilityE.name().c_str());
      } else {
        ImGui::TextDisabled("-");
      }

      ImGui::TableSetColumnIndex(3);
      if (ImGui::SmallButton("View")) {
        showRocketDetailWindow(rocketE);
      }

      ImGui::PopID();
    });

    ImGui::EndTable();
  }
}
