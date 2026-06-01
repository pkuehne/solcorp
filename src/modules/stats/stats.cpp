#include "stats.h"
#include "modules/engine/helpers.h"
#include "modules/lua/lua.h"
#include <format>
#include <modules/base/base.h>

void systemInitialiseStats(flecs::iter &iter);

/// @brief Constructor for the StatsModule.
/// @param[in,out] world The flecs world.
StatsModule::StatsModule(flecs::world &world) {

  world.import <BaseModule>();

  // Register components
  world.component<Stat>("Stat")
      .member("id", &Stat::m_id)
      .member("display", &Stat::m_display)
      .member("description", &Stat::m_description)
      .member("base", &Stat::m_base);
  world.component<Effect>();
  world.component<HasEffect>();
  world.component<Modifier>()
      .member("target_stat", &Modifier::target_stat)
      .member("additive", &Modifier::additive)
      .member("multiplicative", &Modifier::multiplicative);

  // Register lua types
  register_component_lua<Stat>(world, "Stat", [](LuaFieldBuilder<Stat> &b) {
    b.computed<[](const Stat *s) { return s->base(); },
               [](Stat *s, double v) { s->setBase(v); }>("base")
        .getter<[](const Stat *s) { return s->value(); }>("value")
        .getter<[](const Stat *s) { return s->id(); }>("id")
        .getter<[](const Stat *s) { return s->display(); }>("display")
        .getter<[](const Stat *s) { return s->description(); }>("description");
  });

  register_component_lua<Effect>(world, "Effect");

  register_component_lua<Modifier>(
      world, "Modifier", [](LuaFieldBuilder<Modifier> &b) {
        b.field<&Modifier::target_stat>("target_stat")
            .field<&Modifier::additive>("additive")
            .field<&Modifier::multiplicative>("multiplicative");
      });
  // Register Effect category
  auto s = world.set_scope(0);
  world.entity("Effects");
  world.set(s);

  // Register systems
}

double Stat::base() const { return m_base; }
void Stat::setBase(double base) { m_base = base; }

double Stat::value() const {
  return (m_base + m_additive_modifiers) * m_multiplicative_modifiers;
}

void Stat::reset() {
  m_additive_modifiers = 0.0f;
  m_multiplicative_modifiers = 1.0f;
  m_modifiers.clear();
}

const std::string &Stat::id() const { return m_id; }
const std::string &Stat::display() const { return m_display; }
const std::string &Stat::description() const { return m_description; }
const std::vector<EffectModifier> &Stat::modifiers() const {
  return m_modifiers;
}

std::string Stat::format(double value) const {
  switch (m_format) {
  case Format::Currency:
    return "$" + format_locale(value);
  case Format::Percentage:
    return std::format("{:.0f}%", value * 100);
  case Format::Number:
  default:
    return std::format("{:.0f}", value);
  }
}

bool Stat::addModifier(const Modifier &modifier,
                       const std::string &effectName) {
  if (modifier.target_stat != m_id) {
    return false;
  }
  m_additive_modifiers += modifier.additive;
  m_multiplicative_modifiers *= modifier.multiplicative;
  m_modifiers.push_back({modifier, effectName});
  return true;
}

/// @brief Applies modifiers from ancestor entities to the given stats.
///
/// This function traverses the ancestor hierarchy of the given entity and
/// applies any modifiers found in the ancestors to the provided stats. Each
/// stat is reset before applying the modifiers.
///
/// @param e The entity whose ancestors will be traversed.
/// @param stats A vector of pointers to Stat objects that will have modifiers
/// applied.
void applyModifiers(flecs::entity e, std::vector<Stat *> &stats) {
  for (auto *stat : stats) {
    stat->reset();
  }
  for (auto ancestor = e; ancestor.is_valid(); ancestor = ancestor.parent()) {
    ancestor.each<HasEffect>([&](flecs::entity second) {
      second.children([&](flecs::entity modE) {
        const auto *mod = modE.try_get<Modifier>();
        if (mod) {
          const char *effectName = second.name().c_str();
          for (auto *stat : stats) {
            stat->addModifier(*mod, effectName);
          }
        }
      });
    });
  }
}

void statsApplyModifiers(flecs::entity e, Stat *stat) {
  auto vec = std::vector<Stat *>{stat};
  applyModifiers(e, vec);
}
