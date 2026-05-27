#include "launch_window.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/site/site.h"
#include "rocket_module.h"
#include "widgets/widgets.h"
#include <algorithm>
#include <flecs.h>
#include <spdlog/spdlog.h>
#include <utility>

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket,
                         flecs::entity *launchpad) {

  auto window = showWindow(world, "Mission Plan");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Mission Plan");
  auto state = window.try_get_mut<LaunchWindow>();
  SC_ASSERT(state, "Mission Plan state is invalid");

  LaunchScheduleAction draftPlan;
  if (rocket && rocket->is_valid()) {
    draftPlan.rocket = *rocket;
  }
  if (launchpad && launchpad->is_valid()) {
    draftPlan.launchpad = *launchpad;
  }

  showLaunchWindowAdd(world, draftPlan);
}

void showLaunchWindowAdd(flecs::world world, LaunchScheduleAction draftPlan) {

  auto window = showWindow(world, "Mission Plan");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Mission Plan");
  auto state = window.try_get_mut<LaunchWindow>();
  SC_ASSERT(state, "LaunchWindow state is invalid");

  state->draftPlan = std::move(draftPlan);
  state->planningOffset = 0;

  // Default the plan to today; drawLaunchWindow will snap forward once stats
  // are known
  int today = static_cast<int>(world.get<Game>().day);
  if (state->draftPlan.launchDay < today) {
    state->draftPlan.launchDay = today;
  }

  // Set the name if not already set
  if (state->draftPlan.name.empty()) {
    std::string name;
    do { // TODO: Make this a re-usable function
      name = fmt::format("Plan {}", LaunchPlan::max_id++);
    } while (world.lookup(name.c_str()).is_valid());
    state->draftPlan.name = name;
  }
}

/// @brief Draws the Launch Planning window
/// @param winE Entity for the window
/// @param win LaunchWindow component
void drawLaunchWindow(flecs::entity winE) {
  auto &state = winE.get_mut<LaunchWindow>();
  auto world = winE.world();
  flecs::query<> rocketQuery =
      world.query_builder()
          .with<Rocket>()
          .with<RocketCurrentState>(world.lookup("States::Rocket::Stored"))
          // .term(flecs::ChildOf, current_site)
          // .src(flecs::This)
          // .up(flecs::ChildOf)
          .build();
  ;
  flecs::query<Launchpad> launchpadQuery = world.query_builder<Launchpad>()
                                               .with<Facility>()
                                               //  .with(flecs::ChildOf)
                                               //  .second()
                                               //  .var("Site")
                                               .build();

  int today = static_cast<int>(world.get<Game>().day);

  // Derive lead-time stats from current selections
  int prepDays =
      state.draftPlan.launchpad.is_valid()
          ? static_cast<int>(
                state.draftPlan.launchpad.get<Launchpad>().prep_days.value())
          : 0;
  int rolloutDays =
      state.draftPlan.rocket.is_valid()
          ? static_cast<int>(
                state.draftPlan.rocket.get<Rocket>().rollout_days.value())
          : 0;

  // Snap launch date up to the minimum implied by current selections + offset
  int minLaunchDay = today + state.planningOffset + rolloutDays + prepDays;
  if (state.draftPlan.launchDay < minLaunchDay) {
    state.draftPlan.launchDay = minLaunchDay;
  }

  ImGui::Text("Plan Name:");
  ImGui::SameLine();
  ImGui::InputText("##Name", &state.draftPlan.name);

  ImGui::Spacing();

  if (ImGui::BeginTable("##missionLayout", 2,
                        ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("##choices", ImGuiTableColumnFlags_WidthStretch,
                            0.55f);
    ImGui::TableSetupColumn("##timeline", ImGuiTableColumnFlags_WidthStretch,
                            0.45f);
    ImGui::TableNextRow();

    // --- Left column: choices ---
    ImGui::TableSetColumnIndex(0);

    ImGui::Text("Rocket:");
    ImGui::SetNextItemWidth(-1);
    std::string rocketDisplay = state.draftPlan.rocket.is_valid()
                                    ? state.draftPlan.rocket.name()
                                    : "<Select>";
    if (ImGui::BeginCombo("##RocketCombo", rocketDisplay.c_str())) {
      rocketQuery.iter().each([&](flecs::entity e) {
        if (ImGui::Selectable(e.name().c_str(), e == state.draftPlan.rocket)) {
          state.draftPlan.rocket = e;
          // Drop orbit only if the new rocket can't reach it; let validate()
          // catch payload mass overrun
          if (state.draftPlan.targetOrbit.is_valid() &&
              !e.has<CanLiftTo>(state.draftPlan.targetOrbit)) {
            state.draftPlan.targetOrbit = flecs::entity::null();
            state.draftPlan.payloads.clear();
          }
        }
      });
      ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Text("Launchpad:");
    ImGui::SetNextItemWidth(-1);
    std::string padDisplay = state.draftPlan.launchpad.is_valid()
                                 ? state.draftPlan.launchpad.parent().name()
                                 : "<Select>";
    if (ImGui::BeginCombo("##Launchpad", padDisplay.c_str())) {
      launchpadQuery.iter().each([&](flecs::entity e, const Launchpad &) {
        if (ImGui::Selectable(e.parent().name().c_str(),
                              e == state.draftPlan.launchpad)) {
          state.draftPlan.launchpad = e;
        }
      });
      ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Text("Target Orbit:");
    ImGui::SetNextItemWidth(-1);
    std::string orbitDisplay = state.draftPlan.targetOrbit.is_valid()
                                   ? state.draftPlan.targetOrbit.name()
                                   : "<Select>";
    if (ImGui::BeginCombo("##TargetOrbit", orbitDisplay.c_str())) {
      if (state.draftPlan.rocket.is_valid()) {
        state.draftPlan.rocket.each<CanLiftTo>([&](flecs::entity orbit) {
          if (ImGui::Selectable(orbit.name().c_str(),
                                orbit == state.draftPlan.targetOrbit)) {
            state.draftPlan.targetOrbit = orbit;
          }
        });
      }
      ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Text("Planning offset:");
    ImGui::SetNextItemWidth(-1);
    ImGui::DragInt("##Offset", &state.planningOffset, 1.0f, 0, 365, "%d days",
                   ImGuiSliderFlags_AlwaysClamp);

    // --- Right column: stages table ---
    ImGui::TableSetColumnIndex(1);

    // Helper: center-align text within the current table cell
    auto cellCenter = [](const char *text) {
      float pad = std::max(0.0f, (ImGui::GetContentRegionAvail().x -
                                  ImGui::CalcTextSize(text).x) *
                                     0.5f);
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
    };

    if (ImGui::BeginTable(
            "##stages", 2,
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
      ImGui::TableSetupColumn("##dayCol", ImGuiTableColumnFlags_WidthFixed,
                              70.0f);
      ImGui::TableSetupColumn("##stageCol", ImGuiTableColumnFlags_WidthStretch);

      // Manual header row so we can centre "Day"
      ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
      ImGui::TableSetColumnIndex(0);
      cellCenter("Day");
      ImGui::TextUnformatted("Day");
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted("Stage");

      bool canPreview = state.draftPlan.rocket.is_valid() &&
                        state.draftPlan.launchpad.is_valid();
      LaunchPlan preview =
          canPreview ? state.draftPlan.previewPlan() : LaunchPlan{};

      // Rollout row
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      if (canPreview) {
        auto s = std::to_string(preview.rollout_date);
        cellCenter(s.c_str());
        ImGui::TextUnformatted(s.c_str());
      } else {
        cellCenter("—");
        ImGui::TextDisabled("—");
      }
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("Rollout");

      // Pad prep row
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      if (canPreview) {
        auto s = std::to_string(preview.prep_date);
        cellCenter(s.c_str());
        ImGui::TextUnformatted(s.c_str());
      } else {
        cellCenter("—");
        ImGui::TextDisabled("—");
      }
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("Pad Prep");

      // Launch row — editable DragInt fills the cell (widget centres its own
      // text)
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::SetNextItemWidth(-1);
      ImGui::DragInt("##LaunchDate", &state.draftPlan.launchDay, 1.0f,
                     minLaunchDay, today + 1000, "%d",
                     ImGuiSliderFlags_AlwaysClamp);
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("Launch");

      ImGui::EndTable();
    }

    ImGui::EndTable();
  }

  // Payload information and selection
  uint32_t maxMass = 0;
  if (state.draftPlan.rocket.is_valid() &&
      state.draftPlan.targetOrbit.is_valid()) {
    maxMass = state.draftPlan.rocket.get<CanLiftTo>(state.draftPlan.targetOrbit)
                  .max_mass;
  }
  uint32_t totalMass = 0;
  for (const auto &payload : state.draftPlan.payloads) {
    if (payload.is_valid() && payload.has<Payload>()) {
      totalMass += payload.get<Payload>().mass;
    }
  }
  uint32_t remainingMass = (totalMass > maxMass) ? 0 : (maxMass - totalMass);

  ImGui::Separator();
  ImGui::Text("Payloads");
  ImGui::Spacing();

  if (ImGui::BeginTable("##payloadLayout", 2,
                        ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("##payloadList", ImGuiTableColumnFlags_WidthStretch,
                            0.6f);
    ImGui::TableSetupColumn("##payloadMass", ImGuiTableColumnFlags_WidthStretch,
                            0.4f);
    ImGui::TableNextRow();

    // --- Left: selectable payload list ---
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Available Payloads from Contracts:");

    flecs::query<Contract> contractQuery =
        world.query_builder<Contract>().build();

    bool anyPayloads = false;
    contractQuery.each([&](flecs::entity contractE, Contract &contract) {
      if (contract.status != ContractStatus::Accepted) {
        return;
      }
      contractE.each<ContractPayload>([&](flecs::entity payloadE) {
        if (!payloadE.is_valid() && !payloadE.has<Payload>()) {
          return;
        }
        anyPayloads = true;

        bool alreadyAssigned = payloadE.has<LaunchingWith>();

        auto &payload = payloadE.get<Payload>();
        bool isSelected =
            std::ranges::find(state.draftPlan.payloads, payloadE) !=
            state.draftPlan.payloads.end();

        std::string label =
            std::format("{} ({} kg) - {}", payloadE.name().c_str(),
                        payload.mass, contract.client);

        ImGui::BeginDisabled(alreadyAssigned);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
          if (isSelected) {
            std::erase(state.draftPlan.payloads, payloadE);
          } else {
            state.draftPlan.payloads.push_back(payloadE);
          }
        }
        ImGui::EndDisabled();
      });
    });
    if (!anyPayloads) {
      ImGui::TextDisabled("No payloads available");
    }

    // --- Right: mass summary ---
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("Loaded:");
    ImGui::Text("  %u / %u kg", totalMass, maxMass);
    ImGui::Spacing();
    ImGui::Text("Remaining:");
    ImGui::Text("  %u kg", remainingMass);

    ImGui::EndTable();
  }

  ImGui::Spacing();
  ImGui::Spacing();

  auto valid = state.draftPlan.validate(world);
  if (Widgets::ActionButton(
          ButtonLabel{.text = "Save"},
          ButtonTooltip{.text = "Save Launch Plan to be executed"},
          valid.message)) {
    state.draftPlan.execute(world);
    state.draftPlan = LaunchScheduleAction{};
    hideWindow(world, "Mission Plan");
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    state.draftPlan = LaunchScheduleAction{};
    hideWindow(world, "Mission Plan");
  }
}
