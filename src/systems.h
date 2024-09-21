#pragma once

#include "components.h"
#include "graphics.h"

void systemEventHandling(flecs::iter &, size_t, GameResource &, Renderer &);
void systemUpdateSimDate(GameResource &);
// void render_system(flecs::iter &, GameResource *);
// void company_generator_system(flecs::iter &it);
