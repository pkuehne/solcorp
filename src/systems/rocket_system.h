#pragma once

#include "components.h"

void rocket_generator_system(flecs::iter &it);
void rocket_print_system(flecs::iter &it, size_t, const Rocket &);
void rocket_launch_scheduler_system(flecs::iter &it, size_t, const Rocket &);
void launch_decrementer_system(LaunchPlan &);
void launch_end_system(flecs::entity &, LaunchPlan &);
