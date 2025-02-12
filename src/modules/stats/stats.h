#pragma once
#include <flecs.h>
#include <string>
#include <vector>

struct Effect {};
struct HasEffect {};
typedef flecs::pair<HasEffect, Effect> EffectPair;

struct Modifier {
  std::string target_stat;
  double additive = 0.0;
  double multiplicative = 1.0;
};

class Stat {
public:
  Stat(std::string name, double base = 0.0f) : m_name(name), m_base(base) {}

  double base() const;
  double value() const;

  void reset();
  bool addModifier(const Modifier &modifier);

private:
  std::string m_name;
  double m_base = 0.0f;
  double m_additive_modifiers = 0.0f;
  double m_multiplicative_modifiers = 1.0f;
};

void statsApplyModifiers(flecs::entity e, std::vector<Stat *> &stats);
void statsApplyModifiers(flecs::entity e, Stat *stat);

struct StatsModule {
  StatsModule(flecs::world &);
};
