#pragma once

#include <flecs.h>

// Forward declaration to avoid circular dependency with rocket_launch.h
struct Contract;

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

/// @brief Returns true if the accept button should be disabled for the given
/// contract.
/// @param contract The contract to check
/// @return True if the accept button should be disabled, false otherwise
bool acceptButtonDisabled(const Contract &contract);

/// @brief Returns true if the reject button should be disabled for the given
/// contract.
/// @param contract The contract to check
/// @return True if the reject button should be disabled, false otherwise
bool rejectButtonDisabled(const Contract &contract);

/// @brief Returns true if the plan button should be disabled for the given
/// contract.
/// @param contract The contract to check
/// @return True if the plan button should be disabled, false otherwise
bool planButtonDisabled(const Contract &contract);