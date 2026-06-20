#include "stats.h"
#include "modules/engine/helpers.h"
#include "modules/lua/lua.h"
#include <algorithm>
#include <cstddef>
#include <format>
#include <modules/base/base.h>
#include <utility>

void systemInitialiseStats(flecs::iter &iter);
void systemApplyReflectedStats(flecs::iter &iter);
void systemRegisterReflectedStatSystems(flecs::iter &iter);

namespace {

Stat *statAt(void *component, int32_t offset) {
  auto *bytes = static_cast<std::byte *>(component);
  return reinterpret_cast<Stat *>(bytes + offset);
}

const Stat *statAt(const void *component, int32_t offset) {
  auto *bytes = static_cast<const std::byte *>(component);
  return reinterpret_cast<const Stat *>(bytes + offset);
}

std::string statSystemName(flecs::entity component) {
  const auto path = component.path();
  const char *name = path.c_str();
  if (name[0] != '\0') {
    return std::format("Apply Stats {}", name);
  }
  return std::format("Apply Stats #{}", component.id());
}

} // namespace

/// @brief Constructor for the StatsModule.
/// @param[in,out] world The flecs world.
StatsModule::StatsModule(flecs::world &world) {

  world.import <BaseModule>();

  // Register components
  world.component<StatRegistry>().add(flecs::Singleton);
  world.set<StatRegistry>({});
  world.component<StatDef::Format>("StatFormat")
      .constant("Number", StatDef::Format::Number)
      .constant("Currency", StatDef::Format::Currency)
      .constant("Percentage", StatDef::Format::Percentage);
  world.component<StatDef>("StatDef")
      .member("id", &StatDef::id)
      .member("display", &StatDef::display)
      .member("description", &StatDef::description)
      .member("higher_is_better", &StatDef::higher_is_better)
      .member("format", &StatDef::format);
  world.component<Stat>("Stat")
      .member("id", &Stat::m_id)
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
        .getter<[](const Stat *s) { return s->id(); }>("id");
  });
  register_enum_table_lua(world, "StatFormat", [](LuaEnumBuilder &b) {
    b.value("Number", StatDef::Format::Number)
        .value("Currency", StatDef::Format::Currency)
        .value("Percentage", StatDef::Format::Percentage);
  });
  register_component_lua<StatDef>(
      world, "StatDef", [](LuaFieldBuilder<StatDef> &b) {
        b.field<&StatDef::id>("id")
            .field<&StatDef::display>("display")
            .field<&StatDef::description>("description")
            .field<&StatDef::higher_is_better>("higher_is_better")
            .field<&StatDef::format>("format");
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

  world.system("Register Reflected Stat Systems")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemRegisterReflectedStatSystems);
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
const std::vector<EffectModifier> &Stat::modifiers() const {
  return m_modifiers;
}

std::string StatDef::formatValue(double value) const {
  switch (format) {
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
  // Ensure Label is registered before the traversal below: reading it lazily
  // inside each()/children() would be a structural change while the world is
  // mid-iteration, which flecs aborts on. A no-op once Label is registered.
  e.world().component<Label>();
  for (auto ancestor = e; ancestor.is_valid(); ancestor = ancestor.parent()) {
    ancestor.each<HasEffect>([&](flecs::entity second) {
      // Effects are entities named by their stable id; the player-visible name
      // lives in a Label. Prefer it, falling back to the entity name.
      const auto *label = second.try_get<Label>();
      const std::string effectName =
          label ? label->label : second.name().c_str();
      second.children([&](flecs::entity modE) {
        const auto *mod = modE.try_get<Modifier>();
        if (mod) {
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

void discoverStatComponents(flecs::world &world) {
  auto &registry = world.ensure<StatRegistry>();
  const flecs::id_t stat_id = world.component<Stat>().id();

  world.query<const EcsStruct>().each([&](flecs::entity component,
                                          const EcsStruct &metadata) {
    std::vector<int32_t> offsets;
    const int32_t count = ecs_vec_count(&metadata.members);
    offsets.reserve(static_cast<size_t>(count));

    for (int32_t i = 0; i < count; ++i) {
      const auto *member = ecs_vec_get_t(&metadata.members, ecs_member_t, i);
      if (member->type == stat_id) {
        offsets.push_back(member->offset);
      }
    }

    if (offsets.empty()) {
      return;
    }

    std::ranges::sort(offsets);
    auto &entry = registry.components[component.id()];
    entry.component_id = component.id();
    entry.offsets = std::move(offsets);
  });
}

void registerStatSystems(flecs::world &world) {
  discoverStatComponents(world);
  auto &registry = world.ensure<StatRegistry>();

  for (auto &[component_id, entry] : registry.components) {
    if (entry.system_registered) {
      continue;
    }

    auto component = world.entity(component_id);
    const auto name = statSystemName(component);
    world.system(name.c_str())
        .with(component_id)
        .kind(UpdatePhase)
        .run(systemApplyReflectedStats);
    entry.system_registered = true;
  }
}

void registerStatDef(flecs::world &world, StatDef definition) {
  if (definition.id.empty()) {
    return;
  }
  if (definition.display.empty()) {
    definition.display = definition.id;
  }

  auto &registry = world.ensure<StatRegistry>();
  registry.definitions.insert_or_assign(definition.id, std::move(definition));
}

const StatDef *findStatDef(flecs::world &world, std::string_view id) {
  const auto *registry = world.try_get<StatRegistry>();
  if (!registry) {
    return nullptr;
  }

  const auto it = registry->definitions.find(std::string(id));
  if (it == registry->definitions.end()) {
    return nullptr;
  }
  return &it->second;
}

StatDef statDef(flecs::world &world, std::string_view id) {
  if (const auto *definition = findStatDef(world, id)) {
    return *definition;
  }

  const auto fallback = std::string(id);
  return {.id = fallback, .display = fallback, .description = {}};
}

Stat *findStat(flecs::entity e, std::string_view id) {
  if (!e.is_valid()) {
    return nullptr;
  }

  auto world = e.world();
  const auto *registry = world.try_get<StatRegistry>();
  if (!registry) {
    return nullptr;
  }

  for (const auto &[component_id, entry] : registry->components) {
    const void *component = e.try_get(component_id);
    if (!component) {
      continue;
    }

    for (const auto offset : entry.offsets) {
      auto *stat = const_cast<Stat *>(statAt(component, offset));
      if (stat->id() == id) {
        return stat;
      }
    }
  }

  return nullptr;
}

void systemApplyReflectedStats(flecs::iter &iter) {
  auto world = iter.world();
  auto &registry = world.ensure<StatRegistry>();

  std::vector<Stat *> stats;

  while (iter.next()) {
    const flecs::id_t component_id = ecs_field_id(iter.c_ptr(), 0);
    const auto entry_it = registry.components.find(component_id);
    if (entry_it == registry.components.end()) {
      continue;
    }

    const auto &offsets = entry_it->second.offsets;
    stats.reserve(offsets.size());

    for (auto row : iter) {
      auto *component = iter.field_at(0, row);
      stats.clear();

      for (const auto offset : offsets) {
        auto *stat = statAt(component, offset);
        stats.push_back(stat);
      }

      applyModifiers(iter.entity(row), stats);
    }
  }
}

void systemRegisterReflectedStatSystems(flecs::iter &iter) {
  auto world = iter.world();
  registerStatSystems(world);
}
