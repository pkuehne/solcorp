#pragma once
#include "modules/rocket/rocket_module.h"
#include "modules/site/site.h"
#include <flecs.h>

static void run_site_prefab_setup(flecs::world &world) {
  world.import <SiteModule>();
  auto sys = world.system("Setup Site Prefabs")
                 .kind(flecs::OnStart)
                 .immediate()
                 .run(systemCreateSitePrefabs);
  sys.run();
}

static void run_rocket_prefab_setup(flecs::world &world) {
  world.import <RocketModule>();
  auto sys = world.system("Setup Rocket Prefabs")
                 .kind(flecs::OnStart)
                 .immediate()
                 .run(systemCreateRocketPrefabs);
  sys.run();
}
