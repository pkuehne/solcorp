
#include "rocket_launch.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "modules/phase/phase.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include "widgets/widgets.h"
#include <flecs.h>

u_int LaunchPlan::max_id = 1;
u_int Rocket::max_id = 1;

void systemLaunchRocket(flecs::entity, LaunchPlan &);
void systemDrawLaunchWindow(flecs::entity winE, LaunchWindow &);

/// @brief Module Constructor
/// Sets up all necessary components, GUIs and Systems
RocketLaunchModule::RocketLaunchModule(flecs::world &world) {
  spdlog::info("Loading RocketLaunchModule");

  world.import <PhaseModule>();
  world.import <SimulationModule>();
  world.import <SiteModule>();

  // Register components
  world.component<Rocket>();
  world.component<CargoHold>().member<u_int>("capacity");
  world.component<LaunchPlan>();
  world.component<LaunchWindow>()
      .member<u_int>("launchPrepDays")
      .member<int>("launchDay")
      .member<flecs::entity>("planE")
      .member<std::string>("name")
      .member<flecs::entity>("rocket")
      .member<flecs::entity>("launchpad");

  // Register relationships
  world.component<LaunchingWith>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingOn>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingFrom>().add(
      flecs::Symmetric); // Not Exclusive because each Launchpad can have
                         // multiple Plans assigned

  // Register prefabs
  world.prefab<Rocket>().set<CargoHold>({1000});

  // Register systems
  auto sim = world.get<Simulation>();
  world.system<LaunchPlan>("Launch Rocket")
      .tick_source(sim->speed)
      .kind(UpdatePhase)
      .each(systemLaunchRocket);

  world.system<LaunchWindow>("Draw LaunchWindow")
      .kind(GuiPhase)
      .each(systemDrawLaunchWindow);
}

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket,
                         flecs::entity *launchpad) {
  u_int today = world.get<Game>()->day;

  auto planE = world.entity().set<LaunchPlan>({});
  std::string name;
  do { // TODO: Make this a re-usable function
    name = fmt::format("Plan {}", LaunchPlan::max_id++);
  } while (world.lookup(name.c_str()).is_valid());
  planE.set_name(name.c_str());

  auto win = LaunchWindow();
  win.name = name;
  win.planE = planE;
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
    spdlog::error("showing launchwindow can't be done on invalid plan");
    return;
  }

  auto world = planE.world();
  LaunchPlan plan = planE.ensure<LaunchPlan>();

  auto win = LaunchWindow();
  win.planE = planE;
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

/// @brief Process LaunchPlans that are due
/// Ensures that the rocket is destroyed after being launched
/// Also removes the launchplan and clears all relationships
/// @param planE The plan's entity
/// @param plan The plan's component
void systemLaunchRocket(flecs::entity planE, LaunchPlan &plan) {
  auto world = planE.world();
  u_int today = world.get<Game>()->day;

  if (plan.launch_date == 0 || plan.launch_date > today) {
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
  spdlog::info("Removing plan: {} launch_date: {} today: {}", planE.id(),
               plan.launch_date, today);
  planE.destruct();
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
  u_int today = world.get<Game>()->day;
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

  std::string issue;
  if (world.lookup(win.name.c_str()).is_valid()) {
    issue = fmt::format("A Launch Plan named '{}' already exists", win.name);
  }

  if (!win.rocket.is_valid()) {
    issue = "No rocket selected";
  } else if (win.rocket.has<LaunchingOn>(flecs::Wildcard)) {
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
    plan->draft = false;
    win.planE.set_name(win.name.c_str());
    win.planE.add<LaunchingOn>(win.rocket);
    win.planE.add<LaunchingFrom>(win.launchpad);

    hideLaunchWindow(world);
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    LaunchPlan *plan = win.planE.get_mut<LaunchPlan>();
    if (plan && plan->draft) {
      win.planE.destruct();
    }
    hideLaunchWindow(world);
  }
  ImGui::End();
}
