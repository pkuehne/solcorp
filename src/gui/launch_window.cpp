#include "launch_window.h"
#include "components.h"
#include "flecs/addons/cpp/c_types.hpp"
#include "imgui.h"
#include "spdlog/spdlog.h"
#include "widgets.h"

void LaunchWindow::show(const flecs::entity &entity) {
  if (entity == flecs::entity() || !entity.is_alive()) {
    spdlog::error("Called show on LaunchWindow with invalid plan: {}",
                  entity.id());
    m_visible = false;
    return;
  }

  u_int today = entity.world().get<GameResource>()->day;

  m_entity = entity;
  m_visible = true;
  m_launchDay = today + m_launchPrepDays;

  m_rocketQuery = m_entity.world()
                      .query_builder<Rocket>()
                      .with<Rocket>()
                      //  .with(flecs::ChildOf)
                      //  .second()
                      //  .var("Site")
                      .build();
  m_launchpadQuery = m_entity.world()
                         .query_builder<Launchpad, Building>()
                         .with<Building>()
                         .build();
  m_rocketDisplay = "<Select Rocket>";
  m_launchpadDisplay = "<Select Launchpad>";
}

void LaunchWindow::hide() { m_visible = false; }

void LaunchWindow::loadData() {
  //
}

void LaunchWindow::draw(flecs::world &) {
  if (!m_visible)
    return;

  if (m_entity == flecs::entity() || !m_entity.is_alive()) {
    spdlog::error("Opened LaunchWindow on non-existant launch plan");
    hide();
    return;
  }

  // auto *plan = m_entity.get_mut<LaunchPlan>();
  u_int today = m_entity.world().get<GameResource>()->day;
  if (static_cast<u_int>(m_launchDay) < today + m_launchPrepDays) {
    m_launchDay = today + m_launchPrepDays;
  }

  this->loadData();
  ImGui::Begin("Launch Planning");

  ImGui::Text("Launch Date: ");
  ImGui::SameLine();
  ImGui::DragInt("##LaunchDate", &m_launchDay, 1.0f, today + 5, today + 1000,
                 "%d", ImGuiSliderFlags_AlwaysClamp);

  // Rocket
  ImGui::Text("Rocket: ");
  ImGui::SameLine();
  if (ImGui::BeginCombo("##RocketCombo", m_rocketDisplay.c_str())) {
    m_rocketQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e, Rocket) {
          std::string display = fmt::format("Rocket {}", e.id());
          if (ImGui::Selectable(display.c_str(), e == m_rocket)) {
            m_rocketDisplay = display;
            m_rocket = e;
          }
        });
    ImGui::EndCombo();
  }

  // Launchpad
  ImGui::Text("Launchpad: ");
  ImGui::SameLine();
  if (ImGui::BeginCombo("##Launchpad", m_launchpadDisplay.c_str())) {
    m_launchpadQuery
        .iter()
        // .set_var("Site", m_entity)
        .each([&](flecs::entity e, Launchpad, Building &b) {
          if (ImGui::Selectable(b.name.c_str(), e == m_launchpad)) {
            m_launchpadDisplay = b.name;
            m_launchpad = e;
          }
        });
    ImGui::EndCombo();
  }

  std::string issue;
  if (!m_rocket.is_valid()) {
    issue = "No rocket selected";
  } else if (m_rocket.has<LaunchingWith>(flecs::Wildcard)) {
    issue = "Rocket is already planned for a launch";
  }
  if (m_launchpad.is_valid()) {
    m_launchpad.each<LaunchingFrom>([&](flecs::entity p) {
      auto plan = p.get<LaunchPlan>();
      if (plan->launch_date < static_cast<u_int>(m_launchDay) &&
          plan->launch_date >= (m_launchDay - m_launchPrepDays)) {
        issue = "Another launch is already scheduled at that time";
      }
    });
  } else {
    issue = "No launchpad selected";
  }

  if (static_cast<u_int>(m_launchDay) < today + m_launchPrepDays) {
    issue =
        fmt::format("Launch needs to be planned at least {} days in advance",
                    m_launchPrepDays);
  }

  if (ActionButton("Save", "Save Launch Plan to be executed", issue)) {
    // Save LaunchPlan and close window
    LaunchPlan *plan = m_entity.get_mut<LaunchPlan>();
    plan->launch_date = m_launchDay;
    m_entity.add<LaunchingWith>(m_rocket);
    m_entity.add<LaunchingFrom>(m_launchpad);

    hide();
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    hide();
  }
  ImGui::End();
}
