/**
 * @file lua_data.h
 * @brief Reusable reader for Lua files that `return` a data table.
 *
 * @details
 * Many mod assets are plain Lua data: a file that returns a table of values
 * (mod manifests, building/tileset metadata, configs). LuaDataFile loads such a
 * file in a throwaway Lua state and exposes typed, stack-free field access so
 * callers never touch the raw Lua C API. The state is owned for the lifetime of
 * the LuaDataFile, so any LuaTableView / LuaValue handed out stays valid only
 * while the owning LuaDataFile is alive.
 */
#pragma once

#include <functional>
#include <lua.hpp>
#include <optional>
#include <string>

/** @brief Read-only view over a Lua table at a fixed stack index. */
class LuaTableView {
public:
  LuaTableView(lua_State *L, int idx) : L_(L), idx_(lua_absindex(L, idx)) {}

  /** @brief String field, or nullopt if absent / not a string. */
  [[nodiscard]] std::optional<std::string> getString(const char *key) const;
  /** @brief Integer field, or nullopt if absent / not a number. */
  [[nodiscard]] std::optional<long long> getInt(const char *key) const;
  /** @brief Number field, or nullopt if absent / not a number. */
  [[nodiscard]] std::optional<double> getNumber(const char *key) const;
  /** @brief Boolean field, or nullopt if absent / not a boolean. */
  [[nodiscard]] std::optional<bool> getBool(const char *key) const;

  /**
   * @brief Iterate the array part (indices 1..n) of the table-valued field
   * `key`, invoking `fn` for each element. No-op if the field is absent or not
   * a table.
   */
  void forEachArrayElement(
      const char *key,
      const std::function<void(const class LuaValue &)> &fn) const;

private:
  lua_State *L_;
  int idx_;
};

/** @brief Read-only view over a single Lua value at a fixed stack index. */
class LuaValue {
public:
  LuaValue(lua_State *L, int idx) : L_(L), idx_(lua_absindex(L, idx)) {}

  /** @brief The value as a string, or nullopt if it is not a string. */
  [[nodiscard]] std::optional<std::string> asString() const;
  /** @brief The value as a table view, or nullopt if it is not a table. */
  [[nodiscard]] std::optional<LuaTableView> asTable() const;

private:
  lua_State *L_;
  int idx_;
};

/** @brief Loads a Lua file that returns a table; owns the throwaway state. */
class LuaDataFile {
public:
  explicit LuaDataFile(const std::string &path);
  ~LuaDataFile();

  LuaDataFile(const LuaDataFile &) = delete;
  LuaDataFile &operator=(const LuaDataFile &) = delete;

  /** @brief True if the file loaded, ran, and returned a table. */
  [[nodiscard]] bool ok() const { return ok_; }
  /** @brief Human-readable failure reason; empty when ok(). */
  [[nodiscard]] const std::string &error() const { return error_; }

  /** @brief View over the returned root table (valid only when ok()). */
  [[nodiscard]] LuaTableView root() const { return {L_, root_idx_}; }

private:
  lua_State *L_ = nullptr;
  int root_idx_ = 0;
  bool ok_ = false;
  std::string error_;
};
