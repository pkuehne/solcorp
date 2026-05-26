#pragma once
#include <flecs.h>

struct LaunchDetailWindow {
  flecs::entity plan = flecs::entity::null();
};

void showLaunchDetailWindow(flecs::entity planE);
void drawLaunchDetailWindow(flecs::entity winE);

inline constexpr const char *modalCancelLaunch = "Confirm Cancel Launch";

/// Draws the cancel confirmation modal. Call
/// ImGui::OpenPopup(modalCancelLaunch) on the frame the player clicks Cancel,
/// then call this every frame. Returns true if the cancel action was executed.
bool drawLaunchCancelConfirmModal(flecs::entity plan, flecs::world &world);
