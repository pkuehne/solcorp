#include "launch_window.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "modules/base/assert.h"
#include "modules/engine/gui.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "rocket_launch.h"
#include "widgets/widgets.h"
#include <algorithm>
#include <flecs.h>
#include <spdlog/spdlog.h>

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket,
                         flecs::entity *launchpad) {

  auto window = showWindow(world, "Mission Plan");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Mission Plan");
  auto state = window.try_get_mut<LaunchWindow>();
  SC_ASSERT(state, "Mission Plan state is invalid");

  ScheduleLaunchAction draftPlan;
  if (rocket && rocket->is_valid()) {
    draftPlan.rocket = *rocket;
  }
  if (launchpad && launchpad->is_valid()) {
    draftPlan.launchpad = *launchpad;
  }

  showLaunchWindowAdd(world, draftPlan);
}

void showLaunchWindowAdd(flecs::world world, ScheduleLaunchAction draftPlan) {

  auto window = showWindow(world, "Mission Plan");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Mission Plan");
  auto state = window.try_get_mut<LaunchWindow>();
  SC_ASSERT(state, "LaunchWindow state is invalid");

  state->draftPlan = draftPlan;

  // Default the plan to today
  u_int today = world.get<Game>().day;
  if (state->draftPlan.launchDay < static_cast<int>(today)) {
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

void showLaunchWindowEdit(const flecs::entity &planE) {
  spdlog::debug("Showing LaunchWindow");
  if (!planE.is_alive()) {
    spdlog::error("Showing LaunchWindow can't be done on invalid plan");
    return;
  }

  auto world = planE.world();
  LaunchPlan plan = planE.ensure<LaunchPlan>();

  auto window = showWindow(world, "Mission Plan");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Mission Plan");
  auto state = window.try_get_mut<LaunchWindow>();
  SC_ASSERT(state, "BuildingWindow state is invalid");

  state->draftPlan.name = planE.name();
  state->draftPlan.launchDay = plan.launch_date;
  state->draftPlan.rocket = planE.target<LaunchingOn>();
  state->draftPlan.launchpad = planE.target<LaunchingFrom>();
  state->draftPlan.targetOrbit = plan.target_orbit;
  state->draftPlan.current = planE;

  // Load existing payloads
  state->draftPlan.payloads.clear();
  planE.each<LaunchingWith>([&](flecs::entity payload) {
    state->draftPlan.payloads.push_back(payload);
  });
}

/// @brief Draws the Launch Planning window
/// @param winE Entity for the window
/// @param win LaunchWindow component
void drawLaunchWindow(flecs::entity winE) {
  auto &state = winE.get_mut<LaunchWindow>();
  auto world = winE.world();
  flecs::query<> rocketQuery = world.query_builder()
                                   .with<Rocket>()
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

  u_int today = world.get<Game>().day;
  if (static_cast<u_int>(state.draftPlan.launchDay) < today) {
    state.draftPlan.launchDay = today;
  }

  ImGui::Text("Plan Name: ");
  ImGui::SameLine();
  ImGui::InputText(" ", &state.draftPlan.name);

  ImGui::Text("Launch Date: ");
  ImGui::SameLine();
  ImGui::DragInt("##LaunchDate", &state.draftPlan.launchDay, 1.0f, today,
                 today + 1000, "%d", ImGuiSliderFlags_AlwaysClamp);

  // Rocket
  ImGui::Text("Rocket: ");
  ImGui::SameLine();

  std::string rocketDisplay = state.draftPlan.rocket.is_valid()
                                  ? state.draftPlan.rocket.name()
                                  : "<Select>";
  if (ImGui::BeginCombo("##RocketCombo", rocketDisplay.c_str())) {
    rocketQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e) {
          if (ImGui::Selectable(e.name().c_str(),
                                e == state.draftPlan.rocket)) {
            state.draftPlan.rocket = e;
            // Clear target orbit when rocket changes
            state.draftPlan.targetOrbit = flecs::entity::null();
            state.draftPlan.payloads.clear();
          }
        });
    ImGui::EndCombo();
  }

  // Launchpad
  ImGui::Text("Launchpad: ");
  ImGui::SameLine();
  std::string padDisplay = state.draftPlan.launchpad.is_valid()
                               ? state.draftPlan.launchpad.parent().name()
                               : "<Select>";
  if (ImGui::BeginCombo("##Launchpad", padDisplay.c_str())) {
    launchpadQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e, Launchpad) {
          if (ImGui::Selectable(e.parent().name().c_str(),
                                e == state.draftPlan.launchpad)) {
            state.draftPlan.launchpad = e;
          }
        });
    ImGui::EndCombo();
  }

  // Target Orbit Selection
  ImGui::Text("Target Orbit: ");
  ImGui::SameLine();

  std::string orbitDisplay = state.draftPlan.targetOrbit.is_valid()
                                 ? state.draftPlan.targetOrbit.name()
                                 : "<Select>";
  if (ImGui::BeginCombo("##TargetOrbit", orbitDisplay.c_str())) {
    if (state.draftPlan.rocket.is_valid()) {
      // List all orbits this rocket can reach
      state.draftPlan.rocket.each<CanLiftTo>([&](flecs::entity orbit) {
        if (ImGui::Selectable(orbit.name().c_str(),
                              orbit == state.draftPlan.targetOrbit)) {
          state.draftPlan.targetOrbit = orbit;
        }
      });
    }
    ImGui::EndCombo();
  }

  // Payload information and selection
  u_int maxMass = 0;
  if (state.draftPlan.rocket.is_valid() &&
      state.draftPlan.targetOrbit.is_valid()) {
    maxMass = state.draftPlan.rocket.get<CanLiftTo>(state.draftPlan.targetOrbit)
                  .max_mass;
  }

  ImGui::Separator();
  ImGui::Text("Payloads");

  // Display current payload mass
  u_int totalMass = 0;
  for (const auto &payload : state.draftPlan.payloads) {
    if (payload.is_valid() && payload.has<Payload>()) {
      totalMass += payload.get<Payload>().mass;
    }
  }
  u_int remainingMass = (totalMass > maxMass) ? 0 : (maxMass - totalMass);

  ImGui::Text("Current Mass: %u / %u kg", totalMass, maxMass);
  ImGui::Text("Remaining Capacity: %u kg", remainingMass);

  ImGui::Spacing();
  ImGui::Text("Available Payloads from Contracts:");

  // Query for accepted contracts with payloads
  flecs::query<Contract> contractQuery =
      world.query_builder<Contract>().build();

  contractQuery.each([&](flecs::entity contractE, Contract &contract) {
    if (contract.status != ContractStatus::Accepted) {
      return; // Skip non-accepted contracts
    }
    contractE.each<ContractPayload>([&](flecs::entity payloadE) {
      if (!payloadE.is_valid() && !payloadE.has<Payload>()) {
        return; // Skip invalid payloads
      }
      bool alreadyAssigned =
          payloadE.has<LaunchingWith>() &&
          payloadE.target<LaunchingWith>() != state.draftPlan.current;

      auto &payload = payloadE.get<Payload>();
      bool isSelected = std::find(state.draftPlan.payloads.begin(),
                                  state.draftPlan.payloads.end(),
                                  payloadE) != state.draftPlan.payloads.end();

      std::string label = std::string(payloadE.name()) + " (" +
                          std::to_string(payload.mass) + " kg) - " +
                          contract.client;

      ImGui::BeginDisabled(alreadyAssigned);
      if (ImGui::Selectable(label.c_str(), isSelected)) {
        if (isSelected) {
          // Remove payload
          state.draftPlan.payloads.erase(
              std::remove(state.draftPlan.payloads.begin(),
                          state.draftPlan.payloads.end(), payloadE),
              state.draftPlan.payloads.end());
        } else {
          // Add payload
          state.draftPlan.payloads.push_back(payloadE);
        }
        ImGui::EndDisabled();
      }
    });
  });

  ImGui::Separator();

  auto valid = state.draftPlan.validate(world);
  if (ActionButton("Save", "Save Launch Plan to be executed", valid.message)) {
    // Save LaunchPlan and close window
    state.draftPlan.execute(world);
    hideWindow(world, "Mission Plan");
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    // clear draft plan and close window
    state.draftPlan = ScheduleLaunchAction{};
    hideWindow(world, "Mission Plan");
  }
  ImGui::End();
}
