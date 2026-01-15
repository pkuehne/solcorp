#include "launch_window.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "modules/base/assert.h"
#include "modules/engine/gui.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "rocket_launch.h"
#include "widgets/widgets.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket,
                         flecs::entity *launchpad) {
  std::string name;
  do { // TODO: Make this a re-usable function
    name = fmt::format("Plan {}", LaunchPlan::max_id++);
  } while (world.lookup(name.c_str()).is_valid());

  auto window = showWindow(world, "Mission Plan");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Mission Plan");
  auto state = window.try_get_mut<LaunchWindow>();
  SC_ASSERT(state, "BuildingWindow state is invalid");

  state->draftPlan.name = name;
  if (rocket && rocket->is_valid()) {
    state->draftPlan.rocket = *rocket;
  }
  if (launchpad && launchpad->is_valid()) {
    state->draftPlan.launchpad = *launchpad;
  }
  u_int today = world.get<Game>().day;
  state->draftPlan.launchDay = today;
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
  state->draftPlan.current = planE;
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

  auto valid = state.draftPlan.validate(world);
  if (ActionButton("Save", "Save Launch Plan to be executed", valid.message)) {
    // Save LaunchPlan and close window
    state.draftPlan.execute(world);
    hideWindow(world, "Mission Plan");
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    hideWindow(world, "Mission Plan");
  }
  ImGui::End();
}
