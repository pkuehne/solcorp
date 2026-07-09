#include "modules/lua/mod_value.h"

#include <cmath>

ModValue ModValue::Bool(bool value) {
  ModValue v;
  v.type_ = Type::Bool;
  v.bool_ = value;
  return v;
}

ModValue ModValue::Int(long long value) {
  ModValue v;
  v.type_ = Type::Int;
  v.int_ = value;
  return v;
}

ModValue ModValue::Double(double value) {
  ModValue v;
  v.type_ = Type::Double;
  v.double_ = value;
  return v;
}

ModValue ModValue::Str(std::string value) {
  ModValue v;
  v.type_ = Type::String;
  v.string_ = std::move(value);
  return v;
}

ModValue ModValue::Table() {
  ModValue v;
  v.type_ = Type::Table;
  return v;
}

bool ModValue::isDelete() const {
  return type_ == Type::String && string_ == DELETE;
}

std::optional<bool> ModValue::asBool() const {
  if (type_ == Type::Bool) {
    return bool_;
  }
  return std::nullopt;
}

std::optional<long long> ModValue::asInt() const {
  if (type_ == Type::Int) {
    return int_;
  }
  // A double with an exact integral value still reads as an int (Lua-style
  // leniency, mirroring lua_tointeger); a fractional double does not, so we
  // do not silently truncate it.
  if (type_ == Type::Double && std::floor(double_) == double_) {
    return static_cast<long long>(double_);
  }
  return std::nullopt;
}

std::optional<double> ModValue::asNumber() const {
  if (type_ == Type::Double) {
    return double_;
  }
  if (type_ == Type::Int) {
    return static_cast<double>(int_);
  }
  return std::nullopt;
}

std::optional<std::string> ModValue::asString() const {
  if (type_ == Type::String) {
    return string_;
  }
  return std::nullopt;
}

const ModValue *ModValue::find(const char *key) const {
  if (type_ != Type::Table) {
    return nullptr;
  }
  auto it = map_.find(key);
  return it == map_.end() ? nullptr : &it->second;
}

std::optional<std::string> ModValue::getString(const char *key) const {
  const ModValue *v = find(key);
  return v ? v->asString() : std::nullopt;
}

std::optional<long long> ModValue::getInt(const char *key) const {
  const ModValue *v = find(key);
  return v ? v->asInt() : std::nullopt;
}

std::optional<double> ModValue::getNumber(const char *key) const {
  const ModValue *v = find(key);
  return v ? v->asNumber() : std::nullopt;
}

std::optional<bool> ModValue::getBool(const char *key) const {
  const ModValue *v = find(key);
  return v ? v->asBool() : std::nullopt;
}

void ModValue::forEachArrayElement(
    const char *key, const std::function<void(const ModValue &)> &fn) const {
  const ModValue *v = find(key);
  if (v == nullptr || v->type_ != Type::Table) {
    return;
  }
  for (const ModValue &element : v->array_) {
    fn(element);
  }
}

void ModValue::withTable(
    const char *key, const std::function<void(const ModValue &)> &fn) const {
  const ModValue *v = find(key);
  if (v != nullptr && v->type_ == Type::Table) {
    fn(*v);
  }
}

void ModValue::forEachEntry(
    const std::function<void(const std::string &, const ModValue &)> &fn)
    const {
  if (type_ != Type::Table) {
    return;
  }
  for (const auto &[key, value] : map_) {
    fn(key, value);
  }
}

bool ModValue::operator==(const ModValue &other) const {
  if (type_ != other.type_) {
    return false;
  }
  switch (type_) {
  case Type::Nil:
    return true;
  case Type::Bool:
    return bool_ == other.bool_;
  case Type::Int:
    return int_ == other.int_;
  case Type::Double:
    return double_ == other.double_;
  case Type::String:
    return string_ == other.string_;
  case Type::Table:
    return map_ == other.map_ && array_ == other.array_;
  }
  return false;
}
