
#include "rocket_launch.h"
#include "components.h"
#include "imgui.h"
#include "modules/site.h"
#include "spdlog/spdlog.h"
#include "widgets/widgets.h"
#include <flecs.h>

void systemLaunchRocket(flecs::entity, LaunchPlan &);
void systemDrawLaunchWindow(flecs::entity winE, LaunchWindow &win);

/// @brief Module Constructor
/// Sets up all necessary components, GUIs and Systems
RocketLaunchModule::RocketLaunchModule(flecs::world &world) {
  spdlog::info("Loading RocketLaunchModule");

  world.import <SiteModule>();

  flecs::entity UpdatePhase = world.lookup("Phase.Update");
  flecs::entity GuiPhase = world.lookup("Phase.Gui");

  // Register components
  world.component<Rocket>();
  world.component<CargoHold>().member<u_int>("capacity");
  world.component<LaunchPlan>();
  world.component<LaunchWindow>()
      .member<flecs::entity>("planE")
      .member<int>("launchDay")
      .member<u_int>("launchPrepDays")
      .member<flecs::entity>("rocket")
      .member<std::string>("rocketDisplay")
      .member<flecs::entity>("launchpad")
      .member<std::string>("launchpadDisplay");

  // Register relationships
  world.component<LaunchingWith>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingFrom>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingOn>().add(flecs::Exclusive).add(flecs::Symmetric);

  // Register prefabs
  world.prefab<Rocket>().set<CargoHold>({1000});

  // Register systems
  auto game = world.get<GameResource>();
  world.system<LaunchPlan>("Launch Rocket")
      .tick_source(game->sim_speed)
      .kind(UpdatePhase)
      .each(systemLaunchRocket);

  world.system<LaunchWindow>("Draw LaunchWindow")
      .kind(GuiPhase)
      .each(systemDrawLaunchWindow);
}

void showLaunchWindow(const flecs::entity &planE) {
  spdlog::info("Showing LaunchWindow");
  if (!planE.is_alive()) {
    spdlog::error("showing launchwindow can't be done on invalid plan");
    return;
  }

  auto world = planE.world();
  u_int today = world.get<GameResource>()->day;

  auto win = LaunchWindow();
  win.planE = planE;
  win.launchDay = today + win.launchPrepDays;
  world.entity("LaunchWindow").set<LaunchWindow>(win);
}

void hideLaunchWindow(flecs::world &world) {
  spdlog::info("Hiding LaunchWindow");
  auto winE = world.lookup("LaunchWindow");
  winE.destruct();
}

/// @brief Process LaunchPlans that are due
/// Ensures that the rocket is destroyed after being launched
/// Also removes the launchplan and clears all relationships
/// @param planE The plan's entity
/// @param plan The plan's component
void systemLaunchRocket(flecs::entity planE, LaunchPlan &plan) {
  auto world = planE.world();
  u_int today = world.get<GameResource>()->day;

  if (plan.launch_date > today) {
    return;
  }

  auto rocketE = planE.target<LaunchingOn>();
  if (rocketE.is_valid()) {
    spdlog::info("Removing rocket: {}", rocketE.id());
    rocketE.destruct();
  }
  auto payloadE = planE.target<LaunchingWith>();
  if (payloadE.is_valid()) {
    spdlog::info("Removing payload: {}", payloadE.id());

    payloadE.destruct();
  }
  spdlog::info("Removing plan: {}", planE.id());
  planE.destruct();
}

/// @brief Draws the Launch Planning window
/// @param winE Entity for the window
/// @param win LaunchWindow component
void systemDrawLaunchWindow(flecs::entity winE, LaunchWindow &win) {
  spdlog::info("drawingLaunchWindow");
  auto world = winE.world();
  flecs::query<> rocketQuery = world.query_builder()
                                   .with(flecs::IsA)
                                   .second<Rocket>()
                                   //  .with(flecs::ChildOf)
                                   //  .second()
                                   //  .var("Site")
                                   .build();
  ;
  flecs::query<Launchpad, Building> launchpadQuery =
      world.query_builder<Launchpad, Building>()
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
  u_int today = world.get<GameResource>()->day;
  if (static_cast<u_int>(win.launchDay) < today + win.launchPrepDays) {
    win.launchDay = today + win.launchPrepDays;
  }

  ImGui::Begin("Launch Planning");

  ImGui::Text("Launch Date: ");
  ImGui::SameLine();
  ImGui::DragInt("##LaunchDate", &win.launchDay, 1.0f, today + 5, today + 1000,
                 "%d", ImGuiSliderFlags_AlwaysClamp);

  // Rocket
  ImGui::Text("Rocket: ");
  ImGui::SameLine();
  if (ImGui::BeginCombo("##RocketCombo", win.rocketDisplay.c_str())) {
    rocketQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e) {
          std::string display = fmt::format("Rocket {}", e.id());
          if (ImGui::Selectable(display.c_str(), e == win.rocket)) {
            win.rocketDisplay = display;
            win.rocket = e;
          }
        });
    ImGui::EndCombo();
  }

  // Launchpad
  ImGui::Text("Launchpad: ");
  ImGui::SameLine();
  if (ImGui::BeginCombo("##Launchpad", win.launchpadDisplay.c_str())) {
    launchpadQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e, Launchpad, Building &b) {
          if (ImGui::Selectable(b.name.c_str(), e == win.launchpad)) {
            win.launchpadDisplay = b.name;
            win.launchpad = e;
          }
        });
    ImGui::EndCombo();
  }

  std::string issue;
  if (!win.rocket.is_valid()) {
    issue = "No rocket selected";
  } else if (win.rocket.has<LaunchingWith>(flecs::Wildcard)) {
    issue = "Rocket is already planned for a launch";
  }
  if (win.launchpad.is_valid()) {
    win.launchpad.each<LaunchingFrom>([&](flecs::entity p) {
      auto plan = p.get<LaunchPlan>();
      if (plan->launch_date < static_cast<u_int>(win.launchDay) &&
          plan->launch_date >= (win.launchDay - win.launchPrepDays)) {
        issue = "Another launch is already scheduled at that time";
      }
    });
  } else {
    issue = "No launchpad selected";
  }

  if (static_cast<u_int>(win.launchDay) < today + win.launchPrepDays) {
    issue =
        fmt::format("Launch needs to be planned at least {} days in advance",
                    win.launchPrepDays);
  }

  if (ActionButton("Save", "Save Launch Plan to be executed", issue)) {
    // Save LaunchPlan and close window
    LaunchPlan *plan = win.planE.get_mut<LaunchPlan>();
    plan->launch_date = win.launchDay;
    win.planE.add<LaunchingWith>(win.rocket);
    win.planE.add<LaunchingFrom>(win.launchpad);

    hideLaunchWindow(world);
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    hideLaunchWindow(world);
  }
  ImGui::End();
}
