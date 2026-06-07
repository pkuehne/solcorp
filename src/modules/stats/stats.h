#pragma once
#include <cstdint>
#include <flecs.h>
#include <string>
#include <unordered_map>
#include <vector>

/// Represents an effect in the system.
struct Effect {};

/// Relationship tag to idicate an entity has an effect attached.
struct HasEffect {};

/// Represents a modifier that can be applied to a stat.
struct Modifier {
  std::string target_stat;     ///< The stat that the modifier targets.
  double additive = 0.0;       ///< The additive value of the modifier.
  double multiplicative = 1.0; ///< The multiplicative value of the modifier.
};

/// Represents a modifier with an associated effect name.
struct EffectModifier {
  Modifier mod;           ///< The modifier.
  std::string effectName; ///< The name of the effect.
};

struct StatDef {
  std::string id;
  std::string display;
  std::string description;
  bool higher_is_better = true;
  enum class Format : std::uint8_t {
    Number,
    Currency,
    Percentage
  } format = Format::Number;

  [[nodiscard]] std::string formatValue(double value) const;
};

struct StatInit {
  std::string id;
  double base = 0.0;
};

/// Represents a modifiable stat.
class Stat {
public:
  using Format = StatDef::Format;

  Stat() = default;
  explicit Stat(const StatInit &init) : m_id(init.id), m_base(init.base) {}

  /// Gets the base value of the stat.
  /// @return The base value.
  [[nodiscard]] double base() const;
  void setBase(double base);

  /// Gets the current, modified value of the stat.
  /// @return The current value.
  [[nodiscard]] double value() const;

  /// Resets the stat to its base value.
  void reset();

  /// @brief Adds a modifier to the stat.
  ///
  /// @param modifier The modifier to add.
  /// @param effectName The name of the effect associated with the modifier.
  /// @return True if the modifier was added, false otherwise.
  bool addModifier(const Modifier &modifier, const std::string &effectName);

  /// Gets the ID of the stat.
  /// @return The ID.
  [[nodiscard]] const std::string &id() const;

  /// Gets the list of modifiers applied to the stat.
  /// @return The list of modifiers.
  [[nodiscard]] const std::vector<EffectModifier> &modifiers() const;

public:
  std::string m_id;                        ///< The ID of the stat.
  std::vector<EffectModifier> m_modifiers; ///< The list of modifiers.
  double m_base = 0.0f;                    ///< The base value of the stat.
  double m_additive_modifiers = 0.0f;      ///< The total additive modifiers.
  double m_multiplicative_modifiers =
      1.0f; ///< The total multiplicative modifiers.
};

/// Applies modifiers to the stats of an entity.
/// @param e The entity.
/// @param stats The list of stats to apply modifiers to.
void applyModifiers(flecs::entity e, std::vector<Stat *> &stats);

/// Applies modifiers to a single stat of an entity.
/// @param e The entity.
/// @param stat The stat to apply modifiers to.
void statsApplyModifiers(flecs::entity e, Stat *stat);

struct StatComponentRegistration {
  flecs::id_t component_id = 0;
  std::vector<int32_t> offsets;
  bool system_registered = false;
};

struct StatRegistry {
  std::unordered_map<flecs::id_t, StatComponentRegistration> components;
  std::unordered_map<std::string, StatDef> definitions;
};

/// Registers display metadata for a stat id.
void registerStatDef(flecs::world &world, StatDef definition);

/// Finds display metadata for a stat id.
[[nodiscard]] const StatDef *findStatDef(flecs::world &world,
                                         std::string_view id);

/// Gets display metadata for a stat id.
///
/// Unknown ids are represented with a default definition that uses the id as
/// its display name.
[[nodiscard]] StatDef statDef(flecs::world &world, std::string_view id);

/// Discovers reflected component members whose type is Stat.
void discoverStatComponents(flecs::world &world);

/// Registers automatic modifier application systems for discovered stats.
void registerStatSystems(flecs::world &world);

/// Finds a reflected Stat by id on an entity.
///
/// The returned pointer is into ECS component storage. Use it immediately and
/// do not store it across ECS mutations or progress calls.
[[nodiscard]] Stat *findStat(flecs::entity e, std::string_view id);

/// Represents the stats module.
struct StatsModule {
  StatsModule(flecs::world &);
};
