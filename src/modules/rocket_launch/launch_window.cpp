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
  win.draftPlan.name = name;
  if (rocket && rocket->is_valid()) {
    win.draftPlan.rocket = *rocket;
  }
  if (launchpad && launchpad->is_valid()) {
    win.draftPlan.launchpad = *launchpad;
  }
  win.draftPlan.launchDay = today;
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
  win.draftPlan.name = planE.name();
  win.draftPlan.launchDay = plan.launch_date;
  win.draftPlan.rocket = planE.target<LaunchingOn>();
  win.draftPlan.launchpad = planE.target<LaunchingFrom>();
  win.planE = planE;
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

  // bool is_edit = (win.planE == flecs::entity() || !win.planE.is_alive());

  u_int today = world.get<Game>().day;
  if (static_cast<u_int>(win.draftPlan.launchDay) < today) {
    win.draftPlan.launchDay = today;
  }

  ImGui::Begin("Launch Planning");
  ImGui::Text("Plan Name: ");
  ImGui::SameLine();
  ImGui::InputText(" ", &win.draftPlan.name);

  ImGui::Text("Launch Date: ");
  ImGui::SameLine();
  ImGui::DragInt("##LaunchDate", &win.draftPlan.launchDay, 1.0f, today,
                 today + 1000, "%d", ImGuiSliderFlags_AlwaysClamp);

  // Rocket
  ImGui::Text("Rocket: ");
  ImGui::SameLine();

  std::string rocketDisplay = win.draftPlan.rocket.is_valid()
                                  ? win.draftPlan.rocket.name()
                                  : "<Select>";
  if (ImGui::BeginCombo("##RocketCombo", rocketDisplay.c_str())) {
    rocketQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e) {
          if (ImGui::Selectable(e.name().c_str(), e == win.draftPlan.rocket)) {
            win.draftPlan.rocket = e;
          }
        });
    ImGui::EndCombo();
  }

  // Launchpad
  ImGui::Text("Launchpad: ");
  ImGui::SameLine();
  std::string padDisplay = win.draftPlan.launchpad.is_valid()
                               ? win.draftPlan.launchpad.name()
                               : "<Select>";
  if (ImGui::BeginCombo("##Launchpad", padDisplay.c_str())) {
    launchpadQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e, Launchpad) {
          if (ImGui::Selectable(e.name().c_str(),
                                e == win.draftPlan.launchpad)) {
            win.draftPlan.launchpad = e;
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
