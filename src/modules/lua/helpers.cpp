#include "helpers.h"
#include "modules/engine/render.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include <flecs.h>
#include <sol/types.hpp>

flecs::entity create_site(sol::this_state s, const std::string &name,
                          u_int width, u_int height,
                          bool make_current = false) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  auto site = world->entity(name.c_str())
                  .set<Site>({width, height})
                  .set<Transform>({{0, 50}, {}})
                  .add<ConstructionSiteNeedsUpdating>();
  if (make_current) {
    site.add<CurrentSite>();
  }
  return site;
}

flecs::entity create_building(sol::this_state s, const std::string &name,
                              const std::string &prefab, u_int x, u_int y,
                              flecs::entity site) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  std::string prefabName = "Prefabs::Buildings::";
  prefabName.append(prefab);
  auto prefabE = world->lookup(prefabName.c_str());
  if (!prefabE.is_valid()) {
    spdlog::error("Prefab {} does not exist", prefabName);
    return flecs::entity();
  }

  auto entity = world->entity(name.c_str())
                    .is_a(world->lookup(prefabName.c_str()))
                    .set<SiteLocation>({x, y})
                    .child_of(site);
  return entity;
};

flecs::entity create_effect(sol::this_state s, const std::string &name,
                            flecs::entity source) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();
  auto effect = world->entity(name.c_str())
                    .add<Effect>()
                    .child_of((world->lookup("Effects")));
  if (source.is_valid()) {
    source.add<HasEffect>(effect);
  }
  return effect;
};
flecs::entity add_modifier(sol::this_state s, flecs::entity effect,
                           Modifier mod) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  if (!effect.is_valid()) {
    return flecs::entity();
  }
  auto modifier = world->entity().child_of(effect).set<Modifier>(mod);
  return modifier;
};

void load_helpers_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto helpers = solcorp_ns["helpers"].get_or_create<sol::table>();

  helpers.set_function("create_site", create_site);
  helpers.set_function("create_building", create_building);
  helpers.set_function("create_effect", create_effect);
  helpers.set_function("add_modifier", add_modifier);
}
