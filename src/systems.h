#pragma once

#include "components.h"
#include "graphics.h"

void systemEventHandling(flecs::iter &, GameResource *, Renderer *);
void systemUpdateSimDate(flecs::iter &, GameResource *);
// void render_system(flecs::iter &, GameResource *);
// void company_generator_system(flecs::iter &it);
