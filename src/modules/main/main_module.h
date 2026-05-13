#pragma once

#include <flecs.h>
#include <modules/base/base.h>
#include <modules/engine/input.h>

struct MainMenuBar {};

struct MainModule {
public:
  MainModule(flecs::world &);
};

void systemDrawMainMenu(flecs::entity, const Simulation, const Game,
                        MainMenuBar);
void systemToggle(flecs::iter &, size_t, Simulation &, const KeyDown);
void systemTickDurationRequired(flecs::entity, DurationRequired &);
