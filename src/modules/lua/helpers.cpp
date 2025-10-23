#include "helpers.h"
#include "modules/engine/render.h"
#include "modules/site/helpers.h"
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

flecs::entity create_building_prefab(sol::this_state s,
                                     const std::string &name) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  if (!world->lookup("Prefabs")) {
    world->entity("Prefabs");
  }
  if (!world->lookup("Prefabs::Buildings")) {
    world->entity("Buildings").child_of(world->entity("Prefabs"));
  }
  auto buildings = world->lookup("Prefabs::Buildings");
  auto prefab = world->prefab(name.c_str())
                    .is_a(world->lookup("Building"))
                    .child_of(buildings);
  return prefab;
}

flecs::entity create_building(sol::this_state s, const std::string &name,
                              const std::string &prefab, u_int x, u_int y,
                              flecs::entity site) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  return instantiateBuilding(*world, name, prefab, x, y, site);
};

flecs::entity create_texture(sol::this_state s, const std::string &name,
                             const std::string &filename) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  // validate filename
  if (filename.find("..") != std::string::npos) {
    spdlog::error("Invalid filename {}", filename);
    return flecs::entity();
  }
  std::string location = "mods/";
  location.append(mod_state["solcorp"]["mod_name"]());
  location.append("/");
  location.append(filename);
  auto texture_node = world->entity("Textures");
  auto texture = world->entity(name.c_str())
                     .child_of(texture_node)
                     .set<Texture>(loadTexture(location, *world));
  return texture;
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

Sprite clip_sprite_from_texture(sol::this_state s, const std::string &texture,
                                u_int x, u_int y, u_int width, u_int height) {
  sol::state_view mod_state(s);
  auto world = mod_state["solcorp"]["world"].get<flecs::world *>();

  std::string texture_name("Textures::");
  texture_name.append(texture);

  auto textureE = world->lookup(texture_name.c_str());
  if (!textureE.is_valid()) {
    spdlog::error("Texture {} does not exist", texture);
    return Sprite();
  }

  Sprite sprite;
  sprite.texture = textureE;
  sprite.x = x;
  sprite.y = y;
  sprite.width = width;
  sprite.height = height;
  return sprite;
}

void load_helpers_namespace(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto helpers = solcorp_ns["helpers"].get_or_create<sol::table>();

  helpers.set_function("create_building_prefab", create_building_prefab);
  helpers.set_function("create_site", create_site);
  helpers.set_function("create_building", create_building);
  helpers.set_function("create_effect", create_effect);
  helpers.set_function("create_texture", create_texture);
  helpers.set_function("add_modifier", add_modifier);
  helpers.set_function("clip_sprite_from_texture", clip_sprite_from_texture);
}
