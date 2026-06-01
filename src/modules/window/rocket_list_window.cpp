#include "rocket_list_window.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/rocket/rocket_module.h"
#include "modules/site/site.h"
#include "rocket_detail_window.h"
#include <flecs.h>
#include <string>

bool rocketMatchesFilter(flecs::entity rocketE, const RocketListWindow &state) {
  auto stateE = rocketE.target<RocketCurrentState>();
  if (state.hideTerminal && stateE.is_valid() &&
      stateE.has<StateIsTerminal>()) {
    return false;
  }
  if (!state.stateFilter.is_valid()) {
    return true;
  }
  return rocketE.has<RocketCurrentState>(state.stateFilter);
}

void showRocketListWindow(flecs::world &world) {
  showWindow(world, "Rocket List");
}

void drawRocketListWindow(flecs::entity winE) {
  auto &state = winE.get_mut<RocketListWindow>();
  auto world = winE.world();

  // --- Filter ---
  const char *filterPreview = "All";
  if (state.stateFilter.is_valid() && state.stateFilter.is_alive()) {
    filterPreview = state.stateFilter.has<Label>()
                        ? state.stateFilter.get<Label>().label.c_str()
                        : state.stateFilter.name().c_str();
  }
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("##StateFilter", filterPreview)) {
    if (ImGui::Selectable("All", !state.stateFilter.is_valid())) {
      state.stateFilter = flecs::entity::null();
    }
    auto rocketStates = world.lookup("States::Rocket");
    if (rocketStates.is_valid()) {
      rocketStates.children([&](flecs::entity stateE) {
        if (!stateE.has<Label>()) {
          return;
        }
        const char *label = stateE.get<Label>().label.c_str();
        if (ImGui::Selectable(label, state.stateFilter == stateE)) {
          state.stateFilter = stateE;
        }
      });
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Hide retired", &state.hideTerminal);
  ImGui::Separator();

  // --- Table ---
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

    world.query_builder<Rocket>().build().each(
        [&](flecs::entity rocketE, Rocket &) {
          if (!rocketMatchesFilter(rocketE, state)) {
            return;
          }

          ImGui::TableNextRow();
          ImGui::PushID(std::to_string(rocketE.id()).c_str());

          ImGui::TableSetColumnIndex(0);
          ImGui::TextUnformatted(rocketE.name().c_str());

          ImGui::TableSetColumnIndex(1);
          auto stateE = rocketE.target<RocketCurrentState>();
          const char *stateLabel =
              stateE.is_valid() && stateE.has<Label>()
                  ? stateE.get<Label>().label.c_str()
                  : (stateE.is_valid() ? stateE.name().c_str() : "-");
          ImGui::TextUnformatted(stateLabel);
          if (rocketE.has<RocketStateTransitionBlocked>()) {
            const auto &blocked = rocketE.get<RocketStateTransitionBlocked>();
            ImGui::SameLine();
            ImGui::TextColored((ImVec4)ImColor::HSV(0.13f, 1.0f, 1.0f),
                               "\xef\x81\xb1"); // fa-triangle-exclamation f071
            if (ImGui::BeginItemTooltip()) {
              ImGui::Text("%s", blocked.reason.c_str());
              ImGui::EndTooltip();
            }
          }

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
