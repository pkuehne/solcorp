#pragma once
#include "graphics.h"
#include <flecs.h>

void initialiseGUI(flecs::world &);
void systemUpdateUI(flecs::iter &, GameResource *, Renderer *);
void systemRenderUI(flecs::iter &, const Renderer *);
