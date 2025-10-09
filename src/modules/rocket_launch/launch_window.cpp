#include "launch_window.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "rocket_launch.h"
#include "widgets/widgets.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket,
                         flecs::entity *launchpad) {
  u_int today = world.get<Game>().day;

  std::string name;
  do { // TODO: Make this a re-usable function
    name = fmt::format("Plan {}", LaunchPlan::max_id++);
  } while (world.lookup(name.c_str()).is_valid());

  auto win = LaunchWindow();
  win.name = name;
  win.launchDay = today + win.launchPrepDays;
  if (rocket && rocket->is_valid()) {
    win.rocket = *rocket;
  }
  if (launchpad && launchpad->is_valid()) {
    win.launchpad = *launchpad;
  }
  world.entity("LaunchWindow").set<LaunchWindow>(win);
}

void showLaunchWindowEdit(const flecs::entity &planE) {
  spdlog::debug("Showing LaunchWindow");
  if (!planE.is_alive()) {
    spdlog::error("Showing LaunchWindow can't be done on invalid plan");
    return;
  }

  auto world = planE.world();
  LaunchPlan plan = planE.ensure<LaunchPlan>();

  auto win = LaunchWindow();
  win.name = planE.name();
  win.launchDay = plan.launch_date;
  win.rocket = planE.target<LaunchingOn>();
  win.launchpad = planE.target<LaunchingFrom>();
  world.entity("LaunchWindow").set<LaunchWindow>(win);
}

void hideLaunchWindow(flecs::world &world) {
  spdlog::debug("Hiding LaunchWindow");
  auto winE = world.lookup("LaunchWindow");
  winE.destruct();
}

/// @brief Draws the Launch Planning window
/// @param winE Entity for the window
/// @param win LaunchWindow component
void systemDrawLaunchWindow(flecs::entity winE, LaunchWindow &win) {
  auto world = winE.world();
  flecs::query<> rocketQuery = world.query_builder()
                                   .with(flecs::IsA)
                                   .second<Rocket>()
                                   //  .with(flecs::ChildOf)
                                   //  .second()
                                   //  .var("Site")
                                   .build();
  ;
  flecs::query<Launchpad> launchpadQuery = world.query_builder<Launchpad>()
                                               .with<Building>()
                                               //  .with(flecs::ChildOf)
                                               //  .second()
                                               //  .var("Site")
                                               .build();

  if (win.planE == flecs::entity() || !win.planE.is_alive()) {
    spdlog::error("Opened LaunchWindow on non-existant launch plan");
    hideLaunchWindow(world);
    return;
  }

  // auto *plan = m_entity.get_mut<LaunchPlan>();
  u_int today = world.get<Game>().day;
  if (static_cast<u_int>(win.launchDay) < today + win.launchPrepDays) {
    win.launchDay = today + win.launchPrepDays;
  }

  ImGui::Begin("Launch Planning");
  ImGui::Text("Plan Name: ");
  ImGui::SameLine();
  ImGui::InputText(" ", &win.name);

  ImGui::Text("Launch Date: ");
  ImGui::SameLine();
  ImGui::DragInt("##LaunchDate", &win.launchDay, 1.0f, today + 5, today + 1000,
                 "%d", ImGuiSliderFlags_AlwaysClamp);

  // Rocket
  ImGui::Text("Rocket: ");
  ImGui::SameLine();

  std::string rocketDisplay =
      win.rocket.is_valid() ? win.rocket.name() : "<Select>";
  if (ImGui::BeginCombo("##RocketCombo", rocketDisplay.c_str())) {
    rocketQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e) {
          if (ImGui::Selectable(e.name().c_str(), e == win.rocket)) {
            win.rocket = e;
          }
        });
    ImGui::EndCombo();
  }

  // Launchpad
  ImGui::Text("Launchpad: ");
  ImGui::SameLine();
  std::string padDisplay =
      win.launchpad.is_valid() ? win.launchpad.name() : "<Select>";
  if (ImGui::BeginCombo("##Launchpad", padDisplay.c_str())) {
    launchpadQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e, Launchpad) {
          if (ImGui::Selectable(e.name().c_str(), e == win.launchpad)) {
            win.launchpad = e;
          }
        });
    ImGui::EndCombo();
  }

  auto valid = win.draftPlan.validate(world);
  if (ActionButton("Save", "Save Launch Plan to be executed", valid.message)) {
    // Save LaunchPlan and close window
    win.draftPlan.execute(world);
    hideLaunchWindow(world);
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    hideLaunchWindow(world);
  }
  ImGui::End();
}
