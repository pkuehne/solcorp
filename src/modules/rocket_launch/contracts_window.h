#pragma once

#include <flecs.h>

enum class ContractFilterStatus : uint8_t { All = 0, Open, Accepted, Closed };

struct ContractsWindow {
  ContractFilterStatus statusFilter = ContractFilterStatus::All;
  bool showCompleted = true;
  flecs::entity pendingDelete = flecs::entity::null();
};

void showContractsWindow(flecs::world &world);
void drawContractsWindow(flecs::entity winE);

/// @brief Returns true if contract passes the status filter in state.
/// @param contractE The contract entity
/// @param state The window state with active filters
bool contractMatchesFilter(flecs::entity contractE,
                           const ContractsWindow &state);

/// @brief Find or create a launch plan for a contract.
/// Creates a new LaunchPlan, Payload, and sets up the relationships.
/// @param contractE The contract entity
/// @param world The flecs world
flecs::entity setupLaunchForContract(flecs::entity contractE);
