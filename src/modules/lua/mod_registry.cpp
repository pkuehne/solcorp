#include "modules/lua/mod_registry.h"

#include <utility>

namespace {

/**
 * @brief Deep-merge `overrides` (a map-like table) onto `base` (a map-like
 * table), returning whether any value was written and differed. Maps recurse;
 * lists and scalars replace wholesale; a DELETE override removes the field.
 */
bool deep_merge_changed(ModValue &base, const ModValue &overrides) {
  bool changed = false;

  overrides.forEachEntry([&](const std::string &key, const ModValue &ov) {
    auto it = base.fields().find(key);
    const bool exists = it != base.fields().end();

    if (ov.isDelete()) {
      if (exists) {
        base.fields().erase(it);
        changed = true;
      }
      return;
    }

    const bool merge_maps = ov.isTable() && !ov.isArrayLike() && exists &&
                            it->second.isTable() && !it->second.isArrayLike();
    if (merge_maps) {
      if (deep_merge_changed(it->second, ov)) {
        changed = true;
      }
      return;
    }

    // Scalars, lists, new keys, or a type change: replace wholesale.
    if (!exists || it->second != ov) {
      base.fields()[key] = ov;
      changed = true;
    }
  });

  // A mixed table (map keys *and* array elements) also carries a list part,
  // which the map-key walk above never visits. Lists replace wholesale
  // (ADR 011), so an override that supplies array elements replaces the base's;
  // an override with no array part leaves the base list untouched (use DELETE
  // to clear it). Pure lists never reach here - they are replaced as a whole
  // value by the caller - so this only reconciles the array side of a mixed
  // table.
  if (!overrides.array().empty() && base.array() != overrides.array()) {
    base.array() = overrides.array();
    changed = true;
  }

  return changed;
}

} // namespace

void ModRegistry::merge(const std::string &category, const ModValue &mod_table,
                        const std::string &mod_name, int load_order) {
  if (!mod_table.isTable()) {
    return;
  }

  Category &cat = categories_[category];

  mod_table.forEachEntry([&](const std::string &id, const ModValue &def) {
    if (def.isDelete()) {
      // A whole-entry DELETE removes it (and its provenance) from the registry.
      if (cat.data.fields().erase(id) > 0) {
        cat.history.erase(id);
      }
      return;
    }
    if (!def.isTable()) {
      // A definition must be a table; ignore anything else leniently.
      return;
    }

    ModValue &base = cat.data.fields()[id];
    if (!base.isTable()) {
      base = ModValue::Table(); // brand-new entry
    }
    if (deep_merge_changed(base, def)) {
      cat.history[id].push_back(
          ModProvenance{.source = mod_name, .load_order = load_order});
    }
  });
}

const ModValue &ModRegistry::merged(const std::string &category) const {
  static const ModValue kEmpty = ModValue::Table();
  auto it = categories_.find(category);
  return it == categories_.end() ? kEmpty : it->second.data;
}

// (category, id) are both strings and intentionally ordered category-first to
// match the merge()/merged() surface; NOLINT the swappable-params check.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::string ModRegistry::historyString(const std::string &category,
                                       const std::string &id) const {
  auto cat_it = categories_.find(category);
  if (cat_it == categories_.end()) {
    return "";
  }
  auto hist_it = cat_it->second.history.find(id);
  if (hist_it == cat_it->second.history.end()) {
    return "";
  }

  std::string result;
  for (const ModProvenance &touch : hist_it->second) {
    if (!result.empty()) {
      result += " -> ";
    }
    result += touch.source + "(" + std::to_string(touch.load_order) + ")";
  }
  return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::string ModRegistry::historyChain(const std::string &category,
                                      const std::string &id) const {
  auto cat_it = categories_.find(category);
  if (cat_it == categories_.end()) {
    return "";
  }
  auto hist_it = cat_it->second.history.find(id);
  if (hist_it == cat_it->second.history.end()) {
    return "";
  }

  std::string result;
  for (const ModProvenance &touch : hist_it->second) {
    if (!result.empty()) {
      result += " > ";
    }
    result += touch.source;
  }
  return result;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::string ModRegistry::lastSource(const std::string &category,
                                    const std::string &id) const {
  auto cat_it = categories_.find(category);
  if (cat_it == categories_.end()) {
    return "";
  }
  auto hist_it = cat_it->second.history.find(id);
  if (hist_it == cat_it->second.history.end() || hist_it->second.empty()) {
    return "";
  }
  return hist_it->second.back().source;
}
