#include "modules/lua/effect_data.h"

namespace {

Modifier parseModifier(const ModValue &modifier) {
  Modifier mod;
  mod.target_stat = modifier.getString("target_stat").value_or("");
  mod.additive = modifier.getNumber("additive").value_or(0.0);
  mod.multiplicative = modifier.getNumber("multiplicative").value_or(1.0);
  return mod;
}

EffectDef parseEffect(const std::string &id, const ModValue &effect) {
  EffectDef def;
  def.id = id;
  def.name = effect.getString("name").value_or(id);

  effect.forEachArrayElement("modifiers", [&](const ModValue &value) {
    if (value.isTable()) {
      def.modifiers.push_back(parseModifier(value));
    }
  });

  return def;
}

} // namespace

std::vector<EffectDef> parseEffectData(const ModValue &root) {
  std::vector<EffectDef> effects;

  root.forEachEntry([&](const std::string &id, const ModValue &value) {
    if (value.isTable()) {
      effects.push_back(parseEffect(id, value));
    }
  });

  return effects;
}
