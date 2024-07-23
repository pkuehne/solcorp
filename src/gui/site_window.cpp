#include "site_window.h"
#include "components.h"
#include "flecs/addons/cpp/entity.hpp"
#include "imgui.h"
#include "spdlog/spdlog.h"
#include <vector>

bool ActionButton(const char *label, const char *tooltip,
                  const std::string &issue) {
  bool retval = false;
  ImGui::BeginDisabled(!issue.empty());
  if (ImGui::Button(label)) {
    retval = true;
  }
  if ((tooltip || !issue.empty()) && ImGui::BeginItemTooltip()) {
    if (tooltip)
      ImGui::Text("%s", tooltip);
    if (!issue.empty())
      ImGui::TextColored((ImVec4)ImColor::HSV(1.0, 1.0, 1.0), "%s",
                         issue.c_str());
    ImGui::EndTooltip();
  }
  ImGui::EndDisabled();
  return retval;
}

void SiteWindow::loadData() {
  m_manuBuildings.clear();
  m_storageBuildings.clear();

  siteEntity.children([&](flecs::entity e) {
    if (e.has<Manufacturing>()) {
      m_manuBuildings.push_back(e);
    }
    if (e.has<Storage>()) {
      m_storageBuildings.push_back(e);
    }
  });
}

void SiteWindow::draw(flecs::world &) {
  if (!visible)
    return;

  if (siteEntity == flecs::entity() || !siteEntity.is_alive()) {
    spdlog::error("Opened SiteWindow on non-existant site");
    visible = false;
    return;
  }
  this->loadData();

  const Site *site = siteEntity.get<Site>();
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
    if (ActionButton("Schedule", "Schedule the rocket for launch", issue)) {
      ImGui::OpenPopup("Schedule Launch");
    }
    this->movePopup(rocket.parent(), rocket);
    this->schedulePopup(rocket);
  }
  ImGui::PopID();
}

void SiteWindow::drawManufacturingSection() {
  const flecs::world &world = siteEntity.world();

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
    spdlog::info("Picked item: {}", destination.id());
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
