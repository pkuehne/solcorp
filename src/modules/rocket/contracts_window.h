#pragma once

#include <cstdint>
#include <flecs.h>

struct LaunchScheduleAction;

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

/// @brief Creates a launch plan for the given payload and opens the launch
/// window with the plan loaded. The plan will be pre-filled with the payload
/// and target orbit from the contract.
/// @param payloadE The payload entity
LaunchScheduleAction setupLaunchForPayload(flecs::entity payloadE);

/// @brief Returns true if the accept button should be disabled for the given
/// contract entity.
/// @return True if the accept button should be disabled, false otherwise
bool acceptButtonDisabled(flecs::entity contractE);

/// @brief Returns true if the reject button should be disabled for the given
/// contract entity.
/// @return True if the reject button should be disabled, false otherwise
bool rejectButtonDisabled(flecs::entity contractE);

/// @brief Returns true if the plan button should be disabled for the given
/// contract entity.
/// @return True if the plan button should be disabled, false otherwise
bool planButtonDisabled(flecs::entity contractE);

/// @brief Accepts a contract: moves it to Accepted and credits upfront
/// payment to the company balance.
void acceptContract(flecs::world &world, flecs::entity contractE);

/// @brief Rejects a contract: moves it to Closed/failed and refunds the
/// upfront payment if the contract was previously Accepted.
void rejectContract(flecs::world &world, flecs::entity contractE);
