#pragma once

#include "components.h"
#include "graphics.h"
#include <flecs.h>

void systemEventHandling(flecs::iter &, GameResource *, Renderer *);
void render_system(flecs::iter &, GameResource *);
void sim_update_system(flecs::iter &, GameResource *);
void company_generator_system(flecs::iter &it);
