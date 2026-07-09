#include "developer_window.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include "modules/lua/lua.h"
#include "modules/lua/mod_content.h"
#include "modules/simulation/simulation.h"
#include <algorithm>
#include <flecs.h>
#include <spdlog/spdlog.h>
#include <vector>

void drawCheatsTab(flecs::world &);
void drawModsTab(flecs::world &);
void drawFlagsTab(flecs::world &);

void showDeveloperWindow(flecs::world &world) {
  showWindow(world, "Developer Window");
}

void drawPrefabProvenanceTooltip(const flecs::entity &prefabE) {
  auto world = prefabE.world();
  if (!world.has<DebugFlags>() || !world.get<DebugFlags>().prefab_provenance) {
    return;
  }
  if (!prefabE.has<PrefabProvenance>() || !ImGui::IsItemHovered()) {
    return;
  }
  const auto &chain = prefabE.get<PrefabProvenance>().chain;
  ImGui::BeginTooltip();
  // Same amber as the dev-only mod warning below, to read as a dev-tool hint.
  ImGui::TextColored(ImColor(255, 180, 0), "Mods: %s",
                     chain.empty() ? "(unknown)" : chain.c_str());
  ImGui::EndTooltip();
}

void child_tree(flecs::entity e) {
  ImGui::PushID(std::to_string(e.id()).c_str());
  if (ImGui::TreeNode("", "%s", e.name().c_str())) {
    e.children([](flecs::entity e) { child_tree(e); });
    ImGui::TreePop();
  }
  ImGui::PopID();
};

void drawDeveloperWindow(flecs::entity winE) {
  auto &state = winE.get_mut<DeveloperWindow>();
  auto world = winE.world();
  ImGui::Checkbox("Show Metrics Window", &state.show_metrics_window);

  if (ImGui::BeginTabBar("Tools")) {
    if (ImGui::BeginTabItem("Cheats")) {
      drawCheatsTab(world);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Entity Tree")) {
      world.children([](flecs::entity e) { child_tree(e); });
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Mods")) {
      drawModsTab(world);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Flags")) {
      drawFlagsTab(world);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  if (state.show_metrics_window) {
    ImGui::ShowMetricsWindow();
  }
}

void drawCheatsTab(flecs::world &world) {
  if (ImGui::Button("Add $10M to balance")) {
    world.get_mut<Company>().balance += 10'000'000;
  }
}

void drawFlagsTab(flecs::world &world) {
  auto &flags = world.get_mut<DebugFlags>();
  ImGui::Checkbox("Prefab Provenance", &flags.prefab_provenance);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Show each building/rocket prefab's mod-origin chain\n"
        "(e.g. \"core > dev\") as a tooltip on its build button.");
  }
}

void drawModsTab(flecs::world &world) {
  struct ModRow {
    int load_order;
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    bool dev_only;
  };

  std::vector<ModRow> mods;
  world.query_builder<const Mod>().build().each(
      [&](flecs::entity, const Mod &mod) {
        mods.push_back({.load_order = mod.load_order,
                        .id = mod.id,
                        .name = mod.name,
                        .version = mod.version,
                        .author = mod.author,
                        .description = mod.description,
                        .dev_only = mod.dev_only});
      });

  std::ranges::sort(mods, [](const ModRow &lhs, const ModRow &rhs) {
    if (lhs.load_order != rhs.load_order) {
      return lhs.load_order < rhs.load_order;
    }
    return lhs.id < rhs.id;
  });

  // Header row: mod count on the left, reload button right-aligned.
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Loaded mods: %zu", mods.size());

  const char *reloadLabel = "Reload Prefabs";
  const float reloadWidth = ImGui::CalcTextSize(reloadLabel).x +
                            ImGui::GetStyle().FramePadding.x * 2.0f;
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - reloadWidth);
  if (ImGui::Button(reloadLabel)) {
    // Request a reload; an immediate system performs it outside readonly mode,
    // since structural changes are illegal during GUI draw.
    world.add<ReloadPrefabsRequest>();
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Re-reads every mod's textures.lua and buildings.lua from "
        "disk and re-applies them in place. Existing entities keep "
        "their identity, so placed buildings pick up updated "
        "sprites. Entries removed from a data file are not pruned.");
  }

  // ADR 011 §6: warn if a dev-only override mod is active. It is excluded from
  // release builds, so seeing this in a shipped build means one leaked in.
  if (std::ranges::any_of(mods,
                          [](const ModRow &mod) { return mod.dev_only; })) {
    ImGui::TextColored(ImColor(255, 180, 0),
                       "Dev-only override mod active - excluded from release "
                       "builds; not for shipping.");
  }

  ImGui::Separator();

  constexpr ImGuiTableFlags tableFlags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY |
      ImGuiTableFlags_Resizable;

  if (ImGui::BeginTable("loadedMods", 6, tableFlags)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed, 60.0f);
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed, 90.0f);
    ImGui::TableSetupColumn("Author", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("Description");
    ImGui::TableHeadersRow();

    for (const auto &mod : mods) {
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%d", mod.load_order);

      ImGui::TableSetColumnIndex(1);
      if (mod.dev_only) {
        ImGui::TextColored(ImColor(255, 180, 0), "%s (dev)", mod.id.c_str());
      } else {
        ImGui::TextUnformatted(mod.id.c_str());
      }

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(mod.name.c_str());

      ImGui::TableSetColumnIndex(3);
      ImGui::TextUnformatted(mod.version.c_str());

      ImGui::TableSetColumnIndex(4);
      if (mod.author.empty()) {
        ImGui::TextDisabled("-");
      } else {
        ImGui::TextUnformatted(mod.author.c_str());
      }

      ImGui::TableSetColumnIndex(5);
      if (mod.description.empty()) {
        ImGui::TextDisabled("-");
      } else {
        ImGui::TextWrapped("%s", mod.description.c_str());
      }
    }

    ImGui::EndTable();
  }
}
