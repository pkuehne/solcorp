#pragma once

#include "modules/engine/render.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/stats/stats.h"
#include <flecs.h>
#include <lua.hpp>
#include <string>
#include <vector>

struct RocketName {
  std::string value;
};
struct RocketPrefabType {
  std::string value;
};
struct TextureName {
  std::string value;
};
struct TextureFilename {
  std::string value;
};
struct TextureModName {
  std::string value;
};
struct SpriteClipRect {
  int x;
  int y;
  uint32_t width;
  uint32_t height;
};

void load_helpers_namespace(lua_State *L);

flecs::entity create_site(flecs::world world, const std::string &name,
                          uint32_t width, uint32_t height,
                          bool make_current = false);
flecs::entity create_building_prefab(flecs::world world,
                                     const std::string &name);
flecs::entity create_rocket_prefab(flecs::world world, const std::string &name);
flecs::entity add_facility_to_building(flecs::world world,
                                       flecs::entity building,
                                       const std::string &name);
flecs::entity create_rocket(flecs::world world, RocketName name,
                            RocketPrefabType prefab,
                            flecs::entity parent = flecs::entity());
flecs::entity create_building(flecs::world world, const std::string &name,
                              const std::string &prefab, int x, int y,
                              flecs::entity site);
flecs::entity add_target_orbit_to_rocket(flecs::world world,
                                         flecs::entity rocket,
                                         const std::string &orbit_name,
                                         uint32_t max_mass);
flecs::entity create_texture(flecs::world world, TextureName name,
                             TextureFilename filename, TextureModName mod_name);
flecs::entity create_effect(flecs::world world, const std::string &name,
                            flecs::entity source);
flecs::entity add_modifier(flecs::world world, flecs::entity effect,
                           Modifier mod);
Sprite clip_sprite_from_texture(flecs::world world, const std::string &texture,
                                SpriteClipRect rect);
flecs::entity create_contract(flecs::world world, const std::string &name,
                              const std::string &client,
                              const std::string &description,
                              float upfront_payment, float completion_payment);
flecs::entity create_contract_payload(flecs::world world,
                                      flecs::entity contract,
                                      const std::string &name, uint32_t mass,
                                      const std::string &target_orbit_name);
std::vector<flecs::entity> get_all_contracts(flecs::world world);
std::vector<flecs::entity> get_all_active_contracts(flecs::world world);
