#pragma once

#include <flecs.h>
#include <modules/base/base.h>
#include <modules/engine/input.h>

struct WindowModule {
public:
  WindowModule(flecs::world &);
};

void systemToggle(flecs::iter &, size_t, Simulation &, const KeyDown);
void systemTickDurationRequired(flecs::entity, DurationRequired &);
