#pragma once
#include <flecs.h>
#include <map>
#include <string>

struct Stat {
  double base = 0.0f;
  double value = 0.0f;
  bool needsUpdate = true;
};

struct StatType {
  std::string id;
  std::string display;
  std::string description;
  double initial;
  int decimals;
};

struct StatBlock {
  std::map<std::string, Stat> values;
};

StatBlock *stat_get_stat_bock(const flecs::entity &entity);
Stat &stat_get_stat(const std::string &stat_name, const flecs::entity &entity);
int stat_get_base_value(const std::string &stat_name,
                        const flecs::entity &target);
double stat_get_value(const std::string &stat_name,
                      const flecs::entity &entity);
void stat_create_new(flecs::world &world, const std::string &id,
                     const std::string &display, const std::string &description,
                     double initial, int decimals);

void stat_draw_stat(const std::string &stat_name, const flecs::entity &entity);

struct StatsModule {
  StatsModule(flecs::world &);
};
