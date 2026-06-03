#pragma once

#include <flecs.h>

struct ContractListWindow {
  flecs::entity stateFilter = flecs::entity::null();
  bool hideTerminal = true;
};

void showContractListWindow(flecs::world &world);
void drawContractListWindow(flecs::entity winE);

/// @brief Returns true if contract passes the status filter in state.
/// @param contractE The contract entity
/// @param state The window state with active filters
bool contractMatchesFilter(flecs::entity contractE,
                           const ContractListWindow &state);

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
