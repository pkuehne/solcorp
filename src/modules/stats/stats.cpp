#include "stats.h"
#include "imgui.h"
#include <cmath>

void systemInitialiseStats(flecs::iter &iter);

/// @brief Constructor for the StatsModule.
/// @param[in,out] world The flecs world.
StatsModule::StatsModule(flecs::world &world) {
  // Register components
  world.component<Stat>()
      .member<double>("base")
      .member<double>("value")
      .member<bool>("needsUpdate");

  // Register systems
  world.system("Initiatlise GUI")
      .kind(flecs::OnStart)
      .run(systemInitialiseStats);
}

/// @brief Get the StatBlock of an entity.
/// @param[in] entity The flecs entity.
/// @return StatBlock* Pointer to the StatBlock.
StatBlock *stat_get_stat_bock(const flecs::entity &entity) {
  auto world = entity.world();
  flecs::entity UsesStats = world.lookup("Stats::UsesStats");
  flecs::entity stat_parent = entity;
  StatBlock *stats = nullptr;
  while (stat_parent.is_valid()) {
    stats = stat_parent.target(UsesStats).get_mut<StatBlock>();
    if (stats) {
      break;
    }
    stat_parent = stat_parent.parent();
  }
  return stats;
}

/// @brief Get a stat by name from an entity.
/// @param[in] stat_name The name of the stat.
/// @param[in] entity The flecs entity.
/// @return Stat& The stat object.
Stat &stat_get_stat(const std::string &stat_name, const flecs::entity &entity) {
  auto stats = stat_get_stat_bock(entity);
  return stats->values[stat_name];
}

/// @brief Get the base value of a stat.
/// @param[in] stat_name The name of the stat.
/// @param[in] entity The flecs entity.
/// @return int The base value of the stat.
int stat_get_base_value(const std::string &stat_name,
                        const flecs::entity &entity) {
  Stat stat = stat_get_stat(stat_name, entity);
  return stat.base;
}

/// @brief Get the value of a stat.
/// @param[in] stat_name The name of the stat.
/// @param[in] entity The flecs entity.
/// @return double The value of the stat.
double stat_get_value(const std::string &stat_name,
                      const flecs::entity &entity) {
  auto stat = stat_get_stat(stat_name, entity);
  return stat.value;
}

/// @brief Create a new stat.
/// @param[in,out] world The flecs world.
/// @param[in] id The ID of the stat.
/// @param[in] display The display name of the stat.
/// @param[in] description The description of the stat.
/// @param[in] initial The initial value of the stat.
/// @param[in] decimals The number of decimal places to display.
void stat_create_new(flecs::world &world, const std::string &id,
                     const std::string &display, const std::string &description,
                     double initial, int decimals) {
  auto stats_parent = world.lookup("Stats");
  assert(stats_parent);
  world.entity(id.c_str())
      .set<StatType>({id, display, description, initial, decimals})
      .child_of(stats_parent);
}

void stat_draw_stat(const std::string &stat_name, const flecs::entity &entity) {
  auto world = entity.world();
  auto stats = world.lookup("Stats");
  auto definition = stats.lookup(stat_name.c_str()).get_mut<StatType>();
  auto stat = stat_get_stat(stat_name, entity);

  if (definition) {
    ImGui::Text("\uf135 %s: %.*f", definition->display.c_str(),
                definition->decimals, stat.value);
    if (ImGui::BeginItemTooltip()) {
      ImGui::Text("%s", definition->description.c_str());
      ImGui::Separator();
      ImGui::Text("Base Value: %.*f", definition->decimals, stat.base);
      ImGui::Spacing();
      // TODO: Include Effects
      ImGui::Spacing();
      ImGui::Text("%s: ", definition->display.c_str());
      ImGui::SameLine();
      ImGui::Text("%.*f", definition->decimals, stat.value);
      ImGui::EndTooltip();
    }
  } else {
    // Unregistered stat
    ImGui::Text("%s: %f", stat_name.c_str(), stat.value);
  }
}

/// @brief System to initialize stats.
/// @param[in] iter The flecs iterator.
void systemInitialiseStats(flecs::iter &iter) {
  auto world = iter.world();

  auto stats_parent =
      world.entity("Stats"); // Top-level namespace for Stat information
  world.entity("UsesStats").child_of(stats_parent); // StatBlock relationship
  world.entity("HasEffect").child_of(stats_parent); // Effect relationship
}
