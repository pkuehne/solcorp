#include "stats.h"

void systemInitialiseStats(flecs::iter &iter);

/// @brief Constructor for the StatsModule.
/// @param[in,out] world The flecs world.
StatsModule::StatsModule(flecs::world &world) {
  // Register components
  world.component<Effect>();
  world.component<HasEffect>();
  world.component<Modifier>()
      .member<std::string>("target_stat")
      .member<double>("additive")
      .member<double>("multiplicative");

  // Register systems
}

double Stat::base() const { return m_base; }

double Stat::value() const {
  return (m_base + m_additive_modifiers) * m_multiplicative_modifiers;
}

void Stat::reset() {
  m_additive_modifiers = 0.0f;
  m_multiplicative_modifiers = 1.0f;
}

bool Stat::addModifier(const Modifier &modifier) {
  if (modifier.target_stat != m_name) {
    return false;
  }
  m_additive_modifiers += modifier.additive;
  m_multiplicative_modifiers *= modifier.multiplicative;
  return true;
}

void statsApplyModifiers(flecs::entity e, std::vector<Stat *> &stats) {
  for (auto &stat : stats) {
    stat->reset();
  }
  flecs::entity ancestor = e.parent();
  while (ancestor.is_alive()) {
    ancestor.each<HasEffect>([&](flecs::entity second) {
      second.children([&](flecs::entity modE) {
        for (auto &stat : stats) {
          stat->addModifier(*modE.get<Modifier>());
        }
      });
    });
    ancestor = ancestor.parent();
  }
}

void statsApplyModifiers(flecs::entity e, Stat *stat) {
  auto vec = std::vector<Stat *>{stat};
  statsApplyModifiers(e, vec);
}
