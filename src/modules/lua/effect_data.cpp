#include "modules/lua/effect_data.h"

namespace {

Modifier parseModifier(const LuaTableView &modifier) {
  Modifier mod;
  mod.target_stat = modifier.getString("target_stat").value_or("");
  mod.additive = modifier.getNumber("additive").value_or(0.0);
  mod.multiplicative = modifier.getNumber("multiplicative").value_or(1.0);
  return mod;
}

EffectDef parseEffect(const std::string &id, const LuaTableView &effect) {
  EffectDef def;
  def.id = id;
  def.name = effect.getString("name").value_or(id);

  effect.forEachArrayElement("modifiers", [&](const LuaValue &value) {
    if (auto modifier = value.asTable()) {
      def.modifiers.push_back(parseModifier(*modifier));
    }
  });

  return def;
}

} // namespace

std::vector<EffectDef> parseEffectData(const LuaTableView &root) {
  std::vector<EffectDef> effects;

  root.forEachEntry([&](const std::string &id, const LuaValue &value) {
    if (auto effect = value.asTable()) {
      effects.push_back(parseEffect(id, *effect));
    }
  });

  return effects;
}
