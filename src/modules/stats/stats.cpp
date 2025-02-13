#include "stats.h"
#include "imgui.h"
#include "spdlog/fmt/bundled/format.h"

void systemInitialiseStats(flecs::iter &iter);

/// @brief Constructor for the StatsModule.
/// @param[in,out] world The flecs world.
StatsModule::StatsModule(flecs::world &world) {

  // Register components
  world.component<Stat>("Stat")
      .member<std::string>("id")
      .member<std::string>("display")
      .member<std::string>("description")
      .member<double>("base");
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
  m_modifiers.clear();
}

const std::string &Stat::id() const { return m_id; }
const std::string &Stat::display() const { return m_display; }
const std::string &Stat::description() const { return m_description; }
const std::vector<EffectModifier> &Stat::modifiers() const {
  return m_modifiers;
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
  for (auto ancestor = e.parent(); ancestor.is_alive();
       ancestor = ancestor.parent()) {
    ancestor.each<HasEffect>([&](flecs::entity second) {
      second.children([&](flecs::entity modE) {
        const Modifier *mod = modE.get<Modifier>();
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

void displayStatWithTooltip(const Stat *stat) {
  ImGui::Text("%s: %.0f", stat->display().c_str(), stat->value());

  if (ImGui::BeginItemTooltip()) {
    ImGui::Text("%s", stat->description().c_str());
    ImGui::Separator();
    ImGui::Text("Base Value: %.0f", stat->base());
    for (const auto &item : stat->modifiers()) {
      std::string modValue = "";
      auto colour = ImVec4(0.0, 0.5, 0.0, 1.0);
      if (item.mod.additive > 0) {
        modValue = fmt::format("+{:.0f}", item.mod.additive);
      } else if (item.mod.additive < 0) {
        modValue = fmt::format("{:.0f}", item.mod.additive);
        colour = ImVec4(1.0, 0.0, 0.0, 1.0);
      }
      if (item.mod.multiplicative > 1) {
        modValue = fmt::format("+{:.0f}%", (item.mod.multiplicative - 1) * 100);
      } else if (item.mod.multiplicative < 1) {
        modValue = fmt::format("{:.0f}%", (item.mod.multiplicative - 1) * 100);
        colour = ImVec4(1.0, 0.0, 0.0, 1.0);
      }
      ImGui::Text("%s:", item.effectName.c_str());
      ImGui::SameLine();
      ImGui::TextColored(colour, "%s", modValue.c_str());
    }
    ImGui::Separator();
    ImGui::Text("Final Value: %.0f", stat->value());
    ImGui::EndTooltip();
  }
}
