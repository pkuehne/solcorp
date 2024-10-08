#include "site.h"
#include "SDL_keycode.h"
#include "imgui.h"
#include "modules/input/input.h"
#include "modules/phase/phase.h"
#include "modules/render/render.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include "spdlog/spdlog.h"
#include "widgets/widgets.h"

void systemBuildingUpdateConstruction(flecs::entity, Manufacturing &);
void systemDrawSiteWindow(flecs::entity winE, SiteWindow &win);
void movePopup(const flecs::entity &source, flecs::entity &rocket);
void drawRocket(flecs::entity &rocket);
void systemOpenSiteWindow(flecs::iter &, size_t, const KeyDown);
void systemShowBuildingWindow(flecs::entity, Transform &, const MouseUp &);

void systemDrawBuildingWindow(flecs::entity winE, BuildingWindow &win);

SiteModule::SiteModule(flecs::world &world) {

  world.import <PhaseModule>();
  world.import <SimulationModule>();
  world.import <RocketLaunchModule>();

  // Register components
  world.component<Site>()
      .member<std::string>("name")
      .member<u_int>("width")
      .member<u_int>("height");
  world.component<Building>();
  world.component<SiteLocation>().member<u_int>("x").member<u_int>("y");
  world.component<Manufacturing>();
  world.component<Storage>();
  world.component<Office>();
  world.component<Launchpad>();
  world.component<BuildingWindow>().member<flecs::entity>("buildingE");

  // Register Systems
  auto sim = world.get<Simulation>();

  world.system<Manufacturing>("Update Construction")
      .tick_source(sim->speed)
      .kind(UpdatePhase)
      .each(systemBuildingUpdateConstruction);

  world.system<SiteWindow>("Draw Site Window")
      .kind(GuiPhase)
      .each(systemDrawSiteWindow);

  world.system<BuildingWindow>("Draw Building Window")
      .kind(GuiPhase)
      .each(systemDrawBuildingWindow);

  world.system<const KeyDown>("Open Site Window")
      .term_at(0)
      .singleton()
      .kind(ValidatePhase)
      .each(systemOpenSiteWindow);

  world.system<Transform, const MouseUp>("Open Building Window")
      .with<Building>()
      .term_at(1)
      .singleton()
      .kind(ValidatePhase)
      .each(systemShowBuildingWindow);
}

void showSiteWindow(const flecs::entity &siteE) {
  spdlog::info("Showing SiteWindow");
  if (!siteE.is_alive()) {
    spdlog::error("showing launchwindow can't be done on invalid plan");
    return;
  }

  auto world = siteE.world();

  auto win = SiteWindow();
  win.siteE = siteE;
  world.entity("SiteWIndow").set<SiteWindow>(win);
}

void hideSiteWindow(flecs::world &world) {
  spdlog::info("Hiding SiteWindow");
  auto winE = world.lookup("SiteWindow");
  winE.destruct();
}

void systemShowBuildingWindow(flecs::entity e, Transform &t,
                              const MouseUp &mouse) {
  // We know from the query that this is a Building
  int tileSize = 32;
  if ((mouse.x > t.worldPosition.x && mouse.x < t.worldPosition.x + tileSize) &&
      (mouse.y > t.worldPosition.y && mouse.y < t.worldPosition.y + tileSize)) {
    // Click on this building!
    spdlog::info("Clicked on {}", e.name().c_str());
    showBuildingWindow(e);
  }
}

void systemBuildingUpdateConstruction(flecs::entity entity,
                                      Manufacturing &manufacturing) {
  flecs::world world = entity.world();

  entity.children([&](flecs::entity r) {
    Construction *construction = r.get_mut<Construction>();
    if (!construction)
      return;
    if (construction->effort_remaining == 0) {
      r.remove<Construction>();
      return;
    }
    if (manufacturing.available_effort > construction->effort_remaining) {
      construction->effort_remaining = 0;
    } else {
      construction->effort_remaining -= manufacturing.available_effort;
    }
  });
}

void systemDrawSiteWindow(flecs::entity winE, SiteWindow &win) {
  auto world = winE.world();
  if (win.siteE == flecs::entity() || !win.siteE.is_alive()) {
    spdlog::error("Site is no longer valid for SiteWindow");
    hideSiteWindow(world);
    return;
  }

  std::vector<flecs::entity> manuBuildings;
  std::vector<flecs::entity> storageBuildings;

  win.siteE.children([&](flecs::entity e) {
    if (e.has<Manufacturing>()) {
      manuBuildings.push_back(e);
    }
    if (e.has<Storage>()) {
      storageBuildings.push_back(e);
    }
  });

  const Site *site = win.siteE.get<Site>();
  u_int numBuildings = manuBuildings.size() + storageBuildings.size();

  ImGui::Begin("Site");
  ImGui::SeparatorText("General");
  ImGui::Text("Name: %s", site->name.c_str());
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Buildings: %d", numBuildings);
  ImGui::SameLine();
  ImGui::SmallButton("+");

  // drawManufacturingSection();

  ImGui::SeparatorText("Manufacturing");
  for (flecs::entity e : manuBuildings) {
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
        world.entity().is_a<Rocket>().set<Construction>({300, 300}).child_of(e);
      }
      ImGui::Text(" ");
      ImGui::PopID();
    }
    for (flecs::entity r : rockets) {
      drawRocket(r);
    }
    ImGui::PopID();
  }
  // drawStorageSection();
  ImGui::SeparatorText("Storage");
  u_int rocketNum = 0;
  for (flecs::entity &e : storageBuildings) {
    e.children([&](flecs::entity c) {
      if (c.is_a<Rocket>() && !c.has<Construction>()) {
        rocketNum++;
      }
    });
  }
  ImGui::Text("Rockets: %d", rocketNum);

  ImGui::End();
}

void drawRocket(flecs::entity &rocket) {
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
    movePopup(rocket.parent(), rocket);
  }
  ImGui::PopID();
}

void drawManufacturingSection() {}

void drawStorageSection() {}

void movePopup(const flecs::entity &source, flecs::entity &rocket) {
  if (!ImGui::BeginPopupModal("Move Rocket")) {
    return;
  }
  auto world = source.world();

  ImGui::Text("Where to?");
  ImGui::SameLine();
  static flecs::entity destination;
  static std::string display = "<Select one>";

  flecs::query<> storageBuildings = world.query_builder()
                                        .with<Storage>()
                                        .with(flecs::ChildOf, source.parent())
                                        .build();

  if (ImGui::BeginCombo("##StorageCombo", display.c_str())) {
    storageBuildings.each([&](flecs::entity s) {
      if (s == source) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Selectable(s.name().c_str(), destination == s)) {
        destination = s;
        display = s.name();
      }
      if (s == source) {
        ImGui::EndDisabled();
      }
    });
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

void systemOpenSiteWindow(flecs::iter &it, size_t, const KeyDown event) {
  auto world = it.world();
  if (event.key == SDLK_l) {
    showSiteWindow(world.lookup("cape_canaveral"));
  }
}
