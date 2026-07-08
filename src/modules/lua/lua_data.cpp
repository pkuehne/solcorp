#include "lua_data.h"

std::optional<std::string> LuaTableView::getString(const char *key) const {
  lua_getfield(L_, idx_, key);
  std::optional<std::string> result;
  if (lua_type(L_, -1) == LUA_TSTRING) {
    result = std::string(lua_tostring(L_, -1));
  }
  lua_pop(L_, 1);
  return result;
}

std::optional<long long> LuaTableView::getInt(const char *key) const {
  lua_getfield(L_, idx_, key);
  std::optional<long long> result;
  if (lua_type(L_, -1) == LUA_TNUMBER) {
    result = static_cast<long long>(lua_tointeger(L_, -1));
  }
  lua_pop(L_, 1);
  return result;
}

std::optional<double> LuaTableView::getNumber(const char *key) const {
  lua_getfield(L_, idx_, key);
  std::optional<double> result;
  if (lua_type(L_, -1) == LUA_TNUMBER) {
    result = static_cast<double>(lua_tonumber(L_, -1));
  }
  lua_pop(L_, 1);
  return result;
}

std::optional<bool> LuaTableView::getBool(const char *key) const {
  lua_getfield(L_, idx_, key);
  std::optional<bool> result;
  if (lua_type(L_, -1) == LUA_TBOOLEAN) {
    result = lua_toboolean(L_, -1) != 0;
  }
  lua_pop(L_, 1);
  return result;
}

void LuaTableView::forEachArrayElement(
    const char *key, const std::function<void(const LuaValue &)> &fn) const {
  lua_getfield(L_, idx_, key);
  if (lua_istable(L_, -1)) {
    int arr = lua_absindex(L_, -1);
    lua_Integer n = luaL_len(L_, arr);
    for (lua_Integer i = 1; i <= n; ++i) {
      lua_geti(L_, arr, i);
      LuaValue value(L_, lua_gettop(L_));
      fn(value);
      lua_pop(L_, 1);
    }
  }
  lua_pop(L_, 1);
}

void LuaTableView::forEachEntry(
    const std::function<void(const std::string &, const LuaValue &)> &fn)
    const {
  lua_pushnil(L_);
  while (lua_next(L_, idx_) != 0) {
    // key at -2, value at -1. Only handle string keys; calling lua_tostring on
    // a non-string key would mutate it in place and corrupt lua_next.
    if (lua_type(L_, -2) == LUA_TSTRING) {
      std::string entry_key = lua_tostring(L_, -2);
      LuaValue value(L_, lua_gettop(L_));
      fn(entry_key, value);
    }
    lua_pop(L_, 1); // pop value, keep key for the next iteration
  }
}

void LuaTableView::withTable(
    const char *key,
    const std::function<void(const LuaTableView &)> &fn) const {
  lua_getfield(L_, idx_, key);
  if (lua_istable(L_, -1)) {
    LuaTableView view(L_, lua_absindex(L_, -1));
    fn(view);
  }
  lua_pop(L_, 1);
}

std::optional<std::string> LuaValue::asString() const {
  if (lua_type(L_, idx_) == LUA_TSTRING) {
    return std::string(lua_tostring(L_, idx_));
  }
  return std::nullopt;
}

std::optional<LuaTableView> LuaValue::asTable() const {
  if (lua_istable(L_, idx_)) {
    return LuaTableView(L_, idx_);
  }
  return std::nullopt;
}

LuaDataFile::LuaDataFile(const std::string &path) {
  L_ = luaL_newstate();
  if (L_ == nullptr) {
    error_ = "could not create Lua state";
    return;
  }
  luaL_openlibs(L_);

  if (luaL_dofile(L_, path.c_str()) != LUA_OK) {
    const char *err = lua_tostring(L_, -1);
    error_ = err ? err : "could not load '" + path + "'";
    return;
  }

  if (!lua_istable(L_, -1)) {
    error_ = "'" + path + "' did not return a table";
    return;
  }

  root_idx_ = lua_gettop(L_);
  ok_ = true;
}

namespace {

ModValue materializeTable(lua_State *L, int idx);

/// Materialise the Lua value at `idx` into a detached ModValue.
ModValue materializeValue(lua_State *L, int idx) {
  switch (lua_type(L, idx)) {
  case LUA_TBOOLEAN:
    return ModValue::Bool(lua_toboolean(L, idx) != 0);
  case LUA_TNUMBER:
    if (lua_isinteger(L, idx) != 0) {
      return ModValue::Int(static_cast<long long>(lua_tointeger(L, idx)));
    }
    return ModValue::Double(static_cast<double>(lua_tonumber(L, idx)));
  case LUA_TSTRING:
    return ModValue::Str(std::string(lua_tostring(L, idx)));
  case LUA_TTABLE:
    return materializeTable(L, idx);
  default:
    // nil, function, userdata, thread: not representable as data.
    return ModValue{};
  }
}

/// Materialise the Lua table at `idx`: contiguous keys 1..n become the array
/// part, string keys become the map part (matching the LuaTableView readers).
ModValue materializeTable(lua_State *L, int idx) {
  int table = lua_absindex(L, idx);
  ModValue result = ModValue::Table();

  lua_Integer n = luaL_len(L, table);
  for (lua_Integer i = 1; i <= n; ++i) {
    lua_geti(L, table, i);
    result.array().push_back(materializeValue(L, lua_gettop(L)));
    lua_pop(L, 1);
  }

  lua_pushnil(L);
  while (lua_next(L, table) != 0) {
    // key at -2, value at -1. Only handle string keys; calling lua_tostring on
    // a non-string key would mutate it in place and corrupt lua_next.
    if (lua_type(L, -2) == LUA_TSTRING) {
      std::string key = lua_tostring(L, -2);
      result.fields().emplace(std::move(key),
                              materializeValue(L, lua_gettop(L)));
    }
    lua_pop(L, 1); // pop value, keep key for the next iteration
  }

  return result;
}

} // namespace

ModValue LuaDataFile::materialize() const {
  if (!ok_) {
    return {};
  }
  return materializeTable(L_, root_idx_);
}

LuaDataFile::~LuaDataFile() {
  if (L_ != nullptr) {
    lua_close(L_);
  }
}
