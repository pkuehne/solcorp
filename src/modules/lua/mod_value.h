/**
 * @file mod_value.h
 * @brief A detached, recursive value tree for mod data (ADR 011).
 *
 * @details
 * Mod data files are read one at a time in a throwaway Lua state (LuaDataFile),
 * so their tables cannot be deep-merged across mods while they live in
 * separate states. ModValue is a plain C++ representation of a returned Lua
 * data table, materialised out of the Lua state so it outlives it. The registry
 * (mod_registry.h) deep-merges ModValue trees in load order; the typed parsers
 * (building_data.h, etc.) read the merged tree exactly as they used to read a
 * LuaTableView, so their accessors are mirrored here.
 *
 * A ModValue is one of: Nil, Bool, Int, Double, String, or Table. A Table holds
 * both a string-keyed map (mirroring LuaTableView string-key access) and an
 * integer-indexed array part (mirroring 1..n array iteration), so it can carry
 * either a map (e.g. `sprite`, `animations`) or a list (e.g. `facilities`).
 */
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class ModValue {
public:
  enum class Type : std::uint8_t { Nil, Bool, Int, Double, String, Table };

  /// The string value that, as a field override, removes that field on merge.
  static constexpr const char *DELETE = "__DELETE__";

  ModValue() = default; ///< A Nil value.

  static ModValue Bool(bool value);
  static ModValue Int(long long value);
  static ModValue Double(double value);
  static ModValue Str(std::string value);
  static ModValue Table();

  [[nodiscard]] Type type() const { return type_; }
  [[nodiscard]] bool isNil() const { return type_ == Type::Nil; }
  [[nodiscard]] bool isTable() const { return type_ == Type::Table; }
  /// True if this is the string DELETE sentinel.
  [[nodiscard]] bool isDelete() const;

  // Scalar access (nullopt if this value is not of the requested type).
  [[nodiscard]] std::optional<bool> asBool() const;
  [[nodiscard]] std::optional<long long> asInt() const;
  [[nodiscard]] std::optional<double> asNumber() const;
  [[nodiscard]] std::optional<std::string> asString() const;

  // ---- Table field access (mirrors LuaTableView; no-op / nullopt if this is
  // not a Table or the field is absent / of the wrong type). ----
  [[nodiscard]] std::optional<std::string> getString(const char *key) const;
  [[nodiscard]] std::optional<long long> getInt(const char *key) const;
  [[nodiscard]] std::optional<double> getNumber(const char *key) const;
  [[nodiscard]] std::optional<bool> getBool(const char *key) const;

  /// Iterate the array part of the table-valued field `key`.
  void
  forEachArrayElement(const char *key,
                      const std::function<void(const ModValue &)> &fn) const;

  /// Invoke `fn` with the table-valued field `key`, if present and a table.
  void withTable(const char *key,
                 const std::function<void(const ModValue &)> &fn) const;

  /// Iterate this table's own string-keyed entries in key order.
  void forEachEntry(const std::function<void(const std::string &,
                                             const ModValue &)> &fn) const;

  // ---- Mutation / construction (Table only). ----

  /// Mutable string-keyed fields. Empty for non-tables.
  [[nodiscard]] std::map<std::string, ModValue> &fields() { return map_; }
  [[nodiscard]] const std::map<std::string, ModValue> &fields() const {
    return map_;
  }
  /// Mutable array part. Empty for non-tables.
  [[nodiscard]] std::vector<ModValue> &array() { return array_; }
  [[nodiscard]] const std::vector<ModValue> &array() const { return array_; }

  /// True when this table has array elements and no string-keyed fields, i.e.
  /// it represents a list rather than a map. Lists replace wholesale on merge.
  [[nodiscard]] bool isArrayLike() const {
    return type_ == Type::Table && !array_.empty() && map_.empty();
  }

  bool operator==(const ModValue &other) const;
  bool operator!=(const ModValue &other) const { return !(*this == other); }

private:
  /// The string-keyed field `key`, or nullptr if absent or this is not a table.
  [[nodiscard]] const ModValue *find(const char *key) const;

  Type type_ = Type::Nil;
  bool bool_ = false;
  long long int_ = 0;
  double double_ = 0.0;
  std::string string_;
  std::map<std::string, ModValue> map_;
  std::vector<ModValue> array_;
};
