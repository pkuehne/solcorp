
#include "modules/input/input.h"
#include "modules/render/render.h"
#include <flecs.h>

struct Site;
struct Transform;
struct MouseUp;

void systemUpdateConstructionSiteLocations(flecs::entity entity, Site &site);
void systemMatchClickToConstructionSite(flecs::entity e, Transform &t,
                                        const MouseUp &mouse);
