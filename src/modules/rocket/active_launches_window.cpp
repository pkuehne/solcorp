#include "active_launches_window.h"
#include "imgui.h"
#include "launch_actions.h"
#include "launch_detail_window.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "rocket_module.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

bool planMatchesFilters(flecs::entity planE,
                        const ActiveLaunchesWindow &state) {
  flecs::entity padE = planE.target<LaunchingFrom>();
  flecs::entity siteE =
      padE.is_valid() ? findAncestorWith<Site>(padE) : flecs::entity::null();

  flecs::entity payloadE = planE.target<LaunchingWith>();
  flecs::entity orbitE = flecs::entity::null();
  if (payloadE.is_valid()) {
    planE.world().query_builder().with<ContractPayload>(payloadE).build().each(
        [&](flecs::entity contractE) {
          orbitE = contractE.target<ContractTargetOrbit>();
        });
  }

  if (!state.showCompleted) {
    auto currentState = planE.target<LaunchPlanCurrentState>();
    if (currentState.is_valid() && currentState.has<StateIsTerminal>()) {
      return false;
    }
  }

  if (state.filterSite.is_alive() && siteE != state.filterSite)
    return false;
  if (state.filterPad.is_alive() && padE != state.filterPad)
    return false;
  if (state.filterOrbit.is_alive() && orbitE != state.filterOrbit)
    return false;

  return true;
}

void showActiveLaunchesWindow(flecs::world &world) {
  showWindow(world, "Active Launches");
}

void drawActiveLaunchesWindow(flecs::entity winE) {
  auto &state = winE.get_mut<ActiveLaunchesWindow>();
  auto world = winE.world();

  flecs::query<LaunchPlan> planQuery =
      world.query_builder<LaunchPlan>().build();
  flecs::query<Site> siteQuery = world.query_builder<Site>().build();
  flecs::query<Launchpad> padQuery =
      world.query_builder<Launchpad>().with<Facility>().build();
  flecs::query<TargetOrbit> orbitQuery =
      world.query_builder<TargetOrbit>().build();

  // --- Filters ---
  std::string siteDisplay =
      state.filterSite.is_valid() ? state.filterSite.name() : "Any";
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("##SiteFilter", siteDisplay.c_str())) {
    if (ImGui::Selectable("Any", !state.filterSite.is_valid())) {
      state.filterSite = flecs::entity::null();
      state.filterPad = flecs::entity::null();
    }
    siteQuery.each([&](flecs::entity siteE, Site &) {
      if (ImGui::Selectable(siteE.name().c_str(), siteE == state.filterSite)) {
        if (siteE != state.filterSite) {
          state.filterPad = flecs::entity::null();
        }
        state.filterSite = siteE;
      }
    });
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  std::string padDisplay =
      state.filterPad.is_valid() ? state.filterPad.parent().name() : "Any";
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("##PadFilter", padDisplay.c_str())) {
    if (ImGui::Selectable("Any", !state.filterPad.is_valid())) {
      state.filterPad = flecs::entity::null();
    }
    padQuery.each([&](flecs::entity padE, Launchpad &) {
      if (state.filterSite.is_valid() &&
          findAncestorWith<Site>(padE) != state.filterSite) {
        return;
      }
      if (ImGui::Selectable(padE.parent().name().c_str(),
                            padE == state.filterPad)) {
        state.filterPad = padE;
      }
    });
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  std::string orbitDisplay = "Any";
  if (state.filterOrbit.is_valid()) {
    const auto *o = state.filterOrbit.try_get<TargetOrbit>();
    if (o) {
      orbitDisplay = fmt::format("{:.0f} km / {:.1f}°", o->altitude / 1000.0,
                                 o->inclination * 180.0 / std::numbers::pi);
    }
  }
  ImGui::SetNextItemWidth(200.0f);
  if (ImGui::BeginCombo("##OrbitFilter", orbitDisplay.c_str())) {
    if (ImGui::Selectable("Any", !state.filterOrbit.is_valid())) {
      state.filterOrbit = flecs::entity::null();
    }
    orbitQuery.each([&](flecs::entity orbitE, TargetOrbit &orbit) {
      std::string label =
          fmt::format("{} - {:.0f} km / {:.1f}°", orbitE.name().c_str(),
                      orbit.altitude / 1000.0,
                      orbit.inclination * 180.0 / std::numbers::pi);
      if (ImGui::Selectable(label.c_str(), orbitE == state.filterOrbit)) {
        state.filterOrbit = orbitE;
      }
    });
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    state.filterSite = flecs::entity::null();
    state.filterPad = flecs::entity::null();
    state.filterOrbit = flecs::entity::null();
  }

  ImGui::SameLine();
  ImGui::Checkbox("Show Completed", &state.showCompleted);

  ImGui::Separator();

  // --- Table ---
  int planCount = 0;
  bool openCancelModal = false;
  ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_SizingStretchProp |
                               ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("active_launches", 8, tableFlags)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Launch Day", ImGuiTableColumnFlags_WidthFixed,
                            80.0f);
    ImGui::TableSetupColumn("Rocket");
    ImGui::TableSetupColumn("Launchpad");
    ImGui::TableSetupColumn("Site");
    ImGui::TableSetupColumn("Target Orbit");
    ImGui::TableSetupColumn("View", ImGuiTableColumnFlags_WidthFixed, 50.0f);
    ImGui::TableSetupColumn("Cancel", ImGuiTableColumnFlags_WidthFixed, 60.0f);
    ImGui::TableHeadersRow();

    planQuery.each([&](flecs::entity planE, LaunchPlan &plan) {
      if (!planMatchesFilters(planE, state)) {
        return;
      }

      flecs::entity rocketE = planE.target<LaunchingOn>();
      flecs::entity padE = planE.target<LaunchingFrom>();
      flecs::entity siteE = padE.is_valid() ? findAncestorWith<Site>(padE)
                                            : flecs::entity::null();
      flecs::entity payloadE = planE.target<LaunchingWith>();
      flecs::entity orbitE = flecs::entity::null();
      if (payloadE.is_valid()) {
        world.query_builder().with<ContractPayload>(payloadE).build().each(
            [&](flecs::entity contractE) {
              orbitE = contractE.target<ContractTargetOrbit>();
            });
      }

      planCount++;
      ImGui::TableNextRow();
      ImGui::PushID(planE.name().c_str());

      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(planE.name().c_str());

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%u", plan.launch_date);

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(rocketE.is_valid() ? rocketE.name().c_str() : "-");

      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(padE.is_valid() ? padE.parent().name().c_str()
                                             : "-");

      ImGui::TableSetColumnIndex(4);
      ImGui::TextUnformatted(siteE.is_valid() ? siteE.name().c_str() : "-");

      ImGui::TableSetColumnIndex(5);
      if (orbitE.is_valid()) {
        const auto *orbit = orbitE.try_get<TargetOrbit>();
        if (orbit) {
          ImGui::TextUnformatted(
              fmt::format("{:.0f} km / {:.1f}°", orbit->altitude / 1000.0,
                          orbit->inclination * 180.0 / std::numbers::pi)
                  .c_str());
        }
      } else {
        ImGui::TextUnformatted("-");
      }

      ImGui::TableSetColumnIndex(6);
      if (ImGui::SmallButton("View")) {
        showLaunchDetailWindow(planE);
      }

      ImGui::TableSetColumnIndex(7);
      {
        auto currentState = planE.target<LaunchPlanCurrentState>();
        bool isTerminal =
            currentState.is_valid() && currentState.has<StateIsTerminal>();
        ImGui::BeginDisabled(isTerminal);
        if (ImGui::SmallButton("Cancel")) {
          state.pendingCancel = planE;
          openCancelModal = true;
        }
        ImGui::EndDisabled();
      }

      ImGui::PopID();
    });

    ImGui::EndTable();
  }

  // OpenPopup must be called in the same ID scope as BeginPopupModal.
  // We deferred it out of the PushID loop above.
  if (openCancelModal) {
    ImGui::OpenPopup("Confirm Cancel Launch");
  }

  // --- Cancel confirmation modal ---
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("Confirm Cancel Launch", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (state.pendingCancel.is_valid() && state.pendingCancel.is_alive()) {
      ImGui::Text("Cancel launch plan '%s'?",
                  state.pendingCancel.name().c_str());
      ImGui::Text("This action cannot be undone.");
      ImGui::Separator();
      if (ImGui::Button("Yes, Cancel Launch")) {
        LaunchCancelAction{state.pendingCancel}.execute(world);
        state.pendingCancel = flecs::entity::null();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Keep")) {
        state.pendingCancel = flecs::entity::null();
        ImGui::CloseCurrentPopup();
      }
    } else {
      state.pendingCancel = flecs::entity::null();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}
