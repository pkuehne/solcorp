#pragma once

#include "modules/engine/render.h"
#include "modules/site/connector.h"
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

flecs::entity create_site(const flecs::world &world, const std::string &name,
                          uint8_t width, uint8_t height,
                          bool make_current = false);
flecs::entity create_building_prefab(const flecs::world &world,
                                     const std::string &name);
flecs::entity create_rocket_prefab(const flecs::world &world,
                                   const std::string &name);
flecs::entity add_facility_to_building(const flecs::world &world,
                                       flecs::entity building,
                                       const std::string &name);
flecs::entity create_rocket(const flecs::world &world, const RocketName &name,
                            const RocketPrefabType &prefab,
                            flecs::entity parent = flecs::entity());
flecs::entity create_building(flecs::world world, const std::string &name,
                              const std::string &prefab, uint8_t x, uint8_t y,
                              flecs::entity site);
flecs::entity add_target_orbit_to_rocket(const flecs::world &world,
                                         flecs::entity rocket,
                                         const std::string &orbit_name,
                                         uint32_t max_mass);
flecs::entity create_texture(flecs::world world, const TextureName &name,
                             TextureFilename filename,
                             const TextureModName &mod_name);
flecs::entity create_effect(const flecs::world &world, const std::string &name,
                            flecs::entity source);
flecs::entity add_modifier(const flecs::world &world, flecs::entity effect,
                           const Modifier &mod);
Sprite clip_sprite_from_texture(const flecs::world &world,
                                const std::string &texture,
                                SpriteClipRect rect);
flecs::entity create_contract(flecs::world &world, const std::string &name,
                              const std::string &client,
                              const std::string &description,
                              uint32_t upfront_payment,
                              uint32_t completion_payment);
flecs::entity create_contract_payload(const flecs::world &world,
                                      flecs::entity contract,
                                      const std::string &name, uint32_t mass,
                                      const std::string &target_orbit_name);
std::vector<flecs::entity> get_all_contracts(const flecs::world &world);
std::vector<flecs::entity> get_all_active_contracts(const flecs::world &world);
flecs::entity create_connector(flecs::world &world, const std::string &name,
                               ConnectorVariant variant, double rotation,
                               uint8_t x, uint8_t y, flecs::entity site,
                               const std::string &base_tileset,
                               const std::string &markings_tileset);
