#pragma once
#include "modules/rocket/rocket_module.h"
#include "modules/site/site.h"
#include "modules/stats/stats.h"
#include <flecs.h>

inline void run_site_prefab_setup(flecs::world &world) {
  world.import <SiteModule>();
  auto sys = world.system("Setup Site Prefabs")
                 .kind(flecs::OnStart)
                 .immediate()
                 .run(systemCreateSitePrefabs);
  sys.run();
}

inline void run_rocket_prefab_setup(flecs::world &world) {
  world.import <RocketModule>();
  auto sys = world.system("Setup Rocket Prefabs")
                 .kind(flecs::OnStart)
                 .immediate()
                 .run(systemCreateRocketPrefabs);
  sys.run();
}

// StatsModule's constructor creates the Effects node (where effect singletons
// live) and registers the Effect/Modifier components, so importing it is
// enough.
inline void run_effect_setup(flecs::world &world) {
  world.import <StatsModule>();
}
