#pragma once
#include <flecs.h>
#include <string>
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

struct StatInit {
  std::string id;
  std::string display;
  std::string description;
  double base = 0.0;
  bool higher_is_better = true;
};

/// Represents a modifiable stat.
class Stat {
public:
  Stat() = default;
  explicit Stat(const StatInit &init)
      : m_id(init.id), m_display(init.display), m_description(init.description),
        m_base(init.base), higher_is_better(init.higher_is_better) {}

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

  /// Gets the display name of the stat.
  /// @return The display name.
  [[nodiscard]] const std::string &display() const;

  /// Gets the description of the stat.
  /// @return The description.
  [[nodiscard]] const std::string &description() const;

  /// Gets the list of modifiers applied to the stat.
  /// @return The list of modifiers.
  [[nodiscard]] const std::vector<EffectModifier> &modifiers() const;

  /// @brief Checks if higher values of the stat are better.
  /// @return True if higher values are better, false otherwise.
  [[nodiscard]] bool isHigherBetter() const { return higher_is_better; }

public:
  std::string m_id;                        ///< The ID of the stat.
  std::string m_display;                   ///< The display name of the stat.
  std::string m_description;               ///< The description of the stat.
  std::vector<EffectModifier> m_modifiers; ///< The list of modifiers.
  double m_base = 0.0f;                    ///< The base value of the stat.
  double m_additive_modifiers = 0.0f;      ///< The total additive modifiers.
  double m_multiplicative_modifiers =
      1.0f;                     ///< The total multiplicative modifiers.
  bool higher_is_better = true; ///< Indicates if higher values are better.
};

/// Applies modifiers to the stats of an entity.
/// @param e The entity.
/// @param stats The list of stats to apply modifiers to.
void applyModifiers(flecs::entity e, std::vector<Stat *> &stats);

/// Applies modifiers to a single stat of an entity.
/// @param e The entity.
/// @param stat The stat to apply modifiers to.
void statsApplyModifiers(flecs::entity e, Stat *stat);

/// @brief Auto-discover and register stat-update systems using Flecs reflection.
///
/// Iterates every component type registered in @p world that has Flecs struct
/// metadata (i.e. at least one `.member()` call). For each component that
/// contains one or more members of type @c Stat, this function registers a
/// dedicated @c UpdatePhase system that calls @c statsApplyModifiers on every
/// such member for every entity that owns the component.
///
/// This eliminates the need to manually write a per-component update system
/// whenever a new @c Stat field is added to a component.  The only requirements
/// are:
///  1. The component is registered with @c world.component<T>() before this
///     function is called.
///  2. Each @c Stat member is exposed via @c .member("name", &T::field).
///
/// @note Called automatically during @c PostStartPhase by @c StatsModule.
///       Exported so that unit tests can call it directly without relying on
///       the phase trigger timing.
/// @param world The Flecs world.
void discover_stat_update_systems(flecs::world &world);

/// Displays a stat with a tooltip in the UI.
/// @param stat The stat to display.
void displayStatWithTooltip(const Stat *stat);

/// Represents the stats module.
struct StatsModule {
  StatsModule(flecs::world &);
};
