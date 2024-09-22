#include "site_window.h"
#include "components.h"
#include "imgui.h"
#include "modules/rocket_launch.h"
#include "spdlog/spdlog.h"
#include "widgets.h"
#include <flecs.h>
#include <vector>

void SiteWindow::show(const flecs::entity &entity) {
  if (entity == flecs::entity() || !entity.is_alive()) {
    spdlog::error("Called show on SiteWindow with invalid site: {}",
                  entity.id());
    m_visible = false;
    return;
  }
  m_siteEntity = entity;
  m_visible = true;
}

void SiteWindow::hide() {
  m_siteEntity = flecs::entity();
  m_visible = false;
}

void SiteWindow::loadData() {
  m_manuBuildings.clear();
  m_storageBuildings.clear();

  m_siteEntity.children([&](flecs::entity e) {
    if (e.has<Manufacturing>()) {
      m_manuBuildings.push_back(e);
    }
    if (e.has<Storage>()) {
      m_storageBuildings.push_back(e);
    }
  });
}

void SiteWindow::draw(flecs::world &) {
  if (!m_visible)
    return;

  if (m_siteEntity == flecs::entity() || !m_siteEntity.is_alive()) {
    spdlog::error("Site is no longer valid for SiteWindow");
    m_visible = false;
    return;
  }
  this->loadData();

  const Site *site = m_siteEntity.get<Site>();
  u_int numBuildings = m_manuBuildings.size() + m_storageBuildings.size();

  ImGui::Begin("Site");
  ImGui::SeparatorText("General");
  ImGui::Text("Name: %s", site->name.c_str());
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Buildings: %d", numBuildings);
  ImGui::SameLine();
  ImGui::SmallButton("+");

  this->drawManufacturingSection();
  this->drawStorageSection();
  ImGui::End();
}

void SiteWindow::drawRocket(flecs::entity &rocket) {
  ImGui::PushID(rocket.id());

  std::string label = fmt::format("Rocket {}", rocket.id());
  if (ImGui::CollapsingHeader(label.c_str(),
                              ImGuiTreeNodeFlags_Leaf |
                                  ImGuiTreeNodeFlags_CollapsingHeader)) {
    const Construction *c = rocket.get<Construction>();
    bool planned = rocket.has<LaunchingWith>(flecs::Wildcard);

    if (c) {
      float completed = c->effort_total - c->effort_remaining;

      ImGui::ProgressBar(completed / c->effort_total);
      ImGui::SameLine();
      if (ImGui::SmallButton("X")) {
        rocket.remove<Construction>();
      }
    } else {
      ImGui::Text("In Storage");
    }

    std::string issue;
    if (c) {
      issue = "Cannot move rocket while being built";
    }
    if (ActionButton("Move", "Move the rocket to another storage at this site",
                     issue)) {
      ImGui::OpenPopup("Move Rocket");
    }
    ImGui::SameLine();
    if (c) {
      issue = "Cannot schedule rocket while being built";
    }
    if (planned) {
      issue = "Rocket is already scheduled";
    }

    if (ActionButton("Schedule", "Schedule the rocket for launch", issue)) {
      // ImGui::OpenPopup("Schedule Launch");
      auto world = rocket.world();
      auto e = world.entity().set<LaunchPlan>({});
      showLaunchWindow(e);
    }
    this->movePopup(rocket.parent(), rocket);
    this->schedulePopup(rocket);
  }
  ImGui::PopID();
}

void SiteWindow::drawManufacturingSection() {
  const flecs::world &world = m_siteEntity.world();

  ImGui::SeparatorText("Manufacturing");
  for (flecs::entity e : m_manuBuildings) {
    ImGui::PushID(e.id());
    const Manufacturing *m = e.get<Manufacturing>();
    ImGui::Text("Production Lines: %d", m->lines);
    std::vector<flecs::entity> rockets;
    e.children([&](flecs::entity r) { rockets.push_back(r); });
    for (u_int ii = 0; ii < m->lines - rockets.size(); ii++) {
      ImGui::PushID(ii);
      ImGui::Text(" ");
      ImGui::AlignTextToFramePadding();
      ImGui::Text("No rocket being built");
      ImGui::SameLine();
      if (ImGui::Button("Begin")) {
        // Build new rocket
        world.entity()
            .is_a(world.get<PrefabResource>()->rocket)
            .set<Construction>({300, 300})
            .child_of(e);
      }
      ImGui::Text(" ");
      ImGui::PopID();
    }
    for (flecs::entity r : rockets) {
      this->drawRocket(r);
    }
    ImGui::PopID();
  }
}

void SiteWindow::drawStorageSection() {
  ImGui::SeparatorText("Storage");
  u_int rocketNum = 0;
  for (flecs::entity &e : m_storageBuildings) {
    e.children([&](flecs::entity c) {
      if (c.has<Rocket>() && !c.has<Construction>()) {
        rocketNum++;
      }
    });
  }
  ImGui::Text("Rockets: %d", rocketNum);
}

void SiteWindow::movePopup(const flecs::entity &source, flecs::entity &rocket) {
  if (!ImGui::BeginPopupModal("Move Rocket")) {
    return;
  }

  ImGui::Text("Where to?");
  ImGui::SameLine();
  static flecs::entity destination;
  static std::string display = "<Select one>";

  if (ImGui::BeginCombo("##StorageCombo", display.c_str())) {
    for (flecs::entity &s : m_storageBuildings) {
      const Building *building = s.get<Building>();
      if (s == source) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Selectable(building->name.c_str(), destination == s)) {
        destination = s;
        display = building->name;
      }
      if (s == source) {
        ImGui::EndDisabled();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::Separator();
  auto closePopup = [&]() {
    // Reset variables when closing popup
    destination = flecs::entity();
    display = "<Select one>";
    ImGui::CloseCurrentPopup();
  };
  if (ImGui::Button("Cancel")) {
    closePopup();
  }
  ImGui::SameLine();
  std::string issue =
      destination == flecs::entity() ? "Invalid destination" : "";
  if (ActionButton("Ok", nullptr, issue)) {
    rocket.child_of(destination);
    closePopup();
  }
  ImGui::EndPopup();
}
void SiteWindow::schedulePopup(flecs::entity &rocket) {
  if (!ImGui::BeginPopupModal("Schedule Launch")) {
    return;
  }

  spdlog::info("{}", rocket.id());
  ImGui::EndPopup();
}
