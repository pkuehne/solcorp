#pragma once

#include <flecs.h>
#include <modules/base/base.h>
#include <modules/simulation/simulation.h>

struct MainMenuBar {};

void systemDrawMainMenu(flecs::entity, const Simulation, const Game,
                        MainMenuBar);
