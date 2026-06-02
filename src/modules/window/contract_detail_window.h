#pragma once

#include <flecs.h>

struct ContractDetailWindow {
  flecs::entity contract = flecs::entity::null();
};

void showContractDetailWindow(flecs::entity contractE);
void drawContractDetailWindow(flecs::entity winE);
