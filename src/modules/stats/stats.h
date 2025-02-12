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

struct EffectModifier {
  Modifier mod;
  std::string effectName;
};

class Stat {
public:
  Stat(const std::string &name, const std::string &display,
       const std::string &description, double base = 0.0f)
      : m_id(name), m_display(display), m_description(description),
        m_base(base) {}

  double base() const;
  double value() const;

  void reset();
  bool addModifier(const Modifier &modifier, const std::string &effectName);

  const std::string &id() const;
  const std::string &display() const;
  const std::string &description() const;
  const std::vector<EffectModifier> &modifiers() const;

private:
  std::string m_id;
  std::string m_display;
  std::string m_description;
  double m_base = 0.0f;
  double m_additive_modifiers = 0.0f;
  double m_multiplicative_modifiers = 1.0f;
  std::vector<EffectModifier> m_modifiers;
};

void applyModifiers(flecs::entity e, std::vector<Stat *> &stats);
void statsApplyModifiers(flecs::entity e, Stat *stat);
void displayStatWithTooltip(const Stat *stat);

struct StatsModule {
  StatsModule(flecs::world &);
};
