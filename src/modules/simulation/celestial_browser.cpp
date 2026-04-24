#include "celestial_browser.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include "simulation.h"
#include <algorithm>
#include <flecs.h>
#include <spdlog/spdlog.h>

void drawCelestialBodyNode(flecs::entity body, CelestialBrowser &state) {
  if (!body.has<CelestialBody>()) {
    return;
  }
  ImGui::PushID(body.id());
  bool has_children = false;
  body.children([&](flecs::entity) { has_children = true; });

  ImGuiTreeNodeFlags flags =
      has_children
          ? 0
          : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

  if (body.name() == "Sun") {
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
  }

  bool open = ImGui::TreeNodeEx(body.name().c_str(), flags);
  if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    state.selected_body = body;
  }
  if (open && has_children) {
    body.children(
        [&state](flecs::entity e) { drawCelestialBodyNode(e, state); });
    ImGui::TreePop();
  }
  ImGui::PopID();
}

void drawCelestialTree(flecs::world &world, CelestialBrowser &state) {
  auto sun = world.lookup("Sun");
  drawCelestialBodyNode(sun, state);
}

void drawCelestialDetails(flecs::entity selected_body) {
  // Placeholder for details rendering logic
  if (!selected_body.is_valid()) {
    ImGui::Text("Select a body to see details.");
    return;
  }
  CelestialBody cb = selected_body.get<CelestialBody>();
  ImGui::Text("Details for: %s", selected_body.name().c_str());
  ImGui::Separator();

  if (ImGui::BeginTable("body_details", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch,
                            1.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableHeadersRow();

    auto row = [](std::string_view label, std::string_view value) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(label.data());
      ImGui::TableSetColumnIndex(1);
      float w = ImGui::GetColumnWidth();
      float text_w = ImGui::CalcTextSize(value.data()).x;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + w - text_w -
                           ImGui::GetStyle().CellPadding.x * 2);
      ImGui::TextUnformatted(value.data());
    };

    row("Semi-major axis", fmt::format("{:.0f} m", cb.semi_major_axis));
    row("Eccentricity", fmt::format("{:.6f}", cb.eccentricity));
    row("Inclination", fmt::format("{:.4f} rad", cb.inclination));
    row("Longitude of Asc. Node",
        fmt::format("{:.4f} rad", cb.longitude_of_ascending_node));
    row("Argument of Periapsis",
        fmt::format("{:.4f} rad", cb.argument_of_periapsis));
    row("Mean anomaly at epoch",
        fmt::format("{:.0f}", cb.mean_anomaly_at_epoch));
    row("Retrograde", cb.retrograde ? "true" : "false");
    row("Radius", fmt::format("{:.0f} m", cb.radius));
    row("Gravity", fmt::format("{:.3f} m/s²", cb.gravity));
    row("Density", fmt::format("{:.0f} kg/m³", cb.density));
    row("Rotation period",
        fmt::format("{:.1f} h", cb.rotation_period / 3600.0));
    row("Albedo", fmt::format("{:.2f}", cb.albedo));

    ImGui::EndTable();
  }
}

void showCelestialBrowser(flecs::world &world) {
  showWindow(world, "Celestial Browser");
}

void drawCelestialBrowser(flecs::entity winE) {

  CelestialBrowser &state = winE.get_mut<CelestialBrowser>();
  auto world = winE.world();

  // Get full region
  float total_width = ImGui::GetContentRegionAvail().x;
  float left_width = total_width * 0.4f; // initial left pane width
  float splitter_thickness = 4.0f;

  // --- Left Pane (tree)
  ImGui::BeginChild("left_pane", ImVec2(left_width, 0), true);
  drawCelestialTree(world, state);
  ImGui::EndChild();

  // --- Splitter
  ImGui::SameLine();
  ImGui::InvisibleButton("splitter", ImVec2(splitter_thickness, -1));
  if (ImGui::IsItemActive())
    left_width += ImGui::GetIO().MouseDelta.x;
  left_width = std::clamp(left_width, 150.0f, total_width - 150.0f);

  ImGui::SameLine();

  // --- Right Pane (details)
  ImGui::BeginChild("right_pane", ImVec2(0, 0), true);
  drawCelestialDetails(state.selected_body);
  ImGui::EndChild();
}
