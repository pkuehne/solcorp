/**
 * @file mod_registry.h
 * @brief Deep-merge / override registry for mod data (ADR 011 §1-§2).
 *
 * @details
 * Each mod may ship a data file per category (buildings, rockets, ...) that
 * returns an id-keyed map of definitions. ModRegistry folds those tables
 * together in load order: later mods deep-merge onto earlier entries, so a mod
 * can patch a single field (or a nested field) without redefining the whole
 * entry. Merge semantics:
 *
 * - **maps deep-merge** (patch `sprite.x` without touching `sprite.y`);
 * - **lists replace wholesale** (`facilities`, animation `frames`) - a
 *   deliberate simplification of ADR 011's index-wise merge;
 * - a field whose override value is ModValue::DELETE is removed;
 * - an id whose whole override value is DELETE removes the entry.
 *
 * Provenance is tracked per entry (which mods changed it, in what order), but
 * only when a merge actually writes a differing value - enough for a debug
 * screen and for naming the culprit when validation later rejects an entry.
 *
 * ModRegistry is pure C++ (no Lua state); mod tables arrive as ModValue trees
 * (see LuaDataFile::materialize). The merged table is handed to the existing
 * typed parsers unchanged.
 */
#pragma once

#include "modules/lua/mod_value.h"

#include <map>
#include <string>
#include <vector>

/// One mod's touch on a registry entry.
struct ModProvenance {
  std::string source; ///< Mod id.
  int load_order;     ///< Resolved load-order index.
};

class ModRegistry {
public:
  /**
   * @brief Fold a mod's returned table (id -> definition) for a category into
   * the registry, deep-merging onto existing entries and recording provenance
   * for every entry a merge actually changed.
   */
  void merge(const std::string &category, const ModValue &mod_table,
             const std::string &mod_name, int load_order);

  /**
   * @brief The merged table for a category (a ModValue::Table of id ->
   * definition). Returns an empty table for an unknown category.
   */
  [[nodiscard]] const ModValue &merged(const std::string &category) const;

  /**
   * @brief Provenance for an entry as "core(0) -> dev(1)", or "" if unknown.
   * Used to name which mods produced a definition when logging validation
   * failures.
   */
  [[nodiscard]] std::string historyString(const std::string &category,
                                          const std::string &id) const;

private:
  struct Category {
    ModValue data = ModValue::Table(); ///< Merged fields: id -> definition.
    std::map<std::string, std::vector<ModProvenance>>
        history; ///< id -> touches.
  };

  std::map<std::string, Category> categories_;
};
