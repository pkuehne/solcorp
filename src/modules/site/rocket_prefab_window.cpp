#include "rocket_prefab_window.h"
#include "imgui.h"
#include "modules/base/assert.h"
#include "modules/engine/gui.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/stats/stats.h"
#include "spdlog/fmt/bundled/core.h"
#include <cmath>
#include <flecs.h>
#include <spdlog/spdlog.h>
#include <string>

namespace {
void buildRocketFromPrefab(flecs::entity &manufacturingE,
                           const flecs::entity &prefabE) {
  auto world = manufacturingE.world();

  auto rocket = world.entity()
                    .is_a(prefabE)
                    .set<Construction>({300, 300})
                    .child_of(manufacturingE);
  rocket.set_name(fmt::format("Rocket {}", Rocket::max_id++).c_str());
}
} // namespace

void showRocketPrefabWindow(const flecs::entity &entity) {
  spdlog::debug("Showing RocketPrefabWindow");
  if (!entity.is_alive()) {
    spdlog::error(
        "showing RocketPrefabWindow can't be done on invalid manufacturing "
        "facility");
    return;
  }

  auto world = entity.world();
  auto window = showWindow(world, "Rocket Prefab Window");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Rocket Prefab Window");

  auto win = window.try_get_mut<Window>();
  SC_ASSERT(win, "Window state is invalid");
  win->title = fmt::format("Build Rocket ({})", entity.name().c_str());

  auto state = window.try_get_mut<RocketPrefabWindow>();
  SC_ASSERT(state, "RocketPrefabWindow state is invalid");
  state->manufacturingE = entity;
}

void drawRocketPrefabWindow(flecs::entity winE) {
  auto &state = winE.get_mut<RocketPrefabWindow>();
  auto world = winE.world();

  auto manufacturingE = state.manufacturingE;
  if (manufacturingE == flecs::entity() || !manufacturingE.is_alive()) {
    spdlog::error(
        "Manufacturing facility is no longer valid for RocketPrefabWindow");
    hideWindow(world, "Rocket Prefab Window");
    return;
  }

  auto rocketsPrefabs = world.lookup("Prefabs::Rockets");
  if (!rocketsPrefabs.is_valid()) {
    spdlog::error("Failed to load rocket prefabs");
    return;
  }

  auto lowEarthOrbit = world.lookup("Sun::Earth::Low Orbit");

  Company &company = world.get_mut<Company>();
  auto buttonSize = ImGui::GetContentRegionAvail();
  buttonSize.y = 30;

  rocketsPrefabs.children([&](flecs::entity prefabE) {
    if (!prefabE.has<Rocket>()) {
      return;
    }

    ImGui::PushID(prefabE.id());

    std::string maxMassText = "N/A";
    if (lowEarthOrbit.is_valid() && prefabE.has<CanLiftTo>(lowEarthOrbit)) {
      maxMassText =
          std::to_string(prefabE.get<CanLiftTo>(lowEarthOrbit).max_mass);
    }

    const Rocket &rocket = prefabE.get<Rocket>();
    Stat cost = rocket.cost;
    statsApplyModifiers(prefabE, &cost);
    const auto rocketCost = static_cast<int64_t>(std::llround(cost.value()));

    ImGui::Text("%s", prefabE.name().c_str());
    ImGui::TextDisabled("Low Earth Orbit: %s kg", maxMassText.c_str());
    displayStatWithTooltip(&cost);

    const bool canAfford = company.balance >= rocketCost;
    ImGui::BeginDisabled(!canAfford);
    if (ImGui::Button("Build", buttonSize)) {
      buildRocketFromPrefab(manufacturingE, prefabE);
      company.balance -= rocketCost;
      hideWindow(world, "Rocket Prefab Window");
    }
    ImGui::EndDisabled();

    if (!canAfford &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      ImGui::SetTooltip("Not enough funds");
    }

    ImGui::Separator();
    ImGui::PopID();
  });
}
