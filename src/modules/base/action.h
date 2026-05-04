#pragma once

#include <flecs.h>
#include <string>

struct ValidationResult {
  bool ok = false;
  std::string message;

  static ValidationResult Pass() { return {.ok = true, .message = {}}; }
  static ValidationResult Fail(const std::string &msg) {
    return {.ok = false, .message = msg};
  }
  static ValidationResult Issue(const std::string &msg) {
    if (!msg.empty()) {
      return Fail(msg);
    }
    return Pass();
  }
  explicit operator bool() const noexcept { return ok; }
};

struct IAction {
  virtual ~IAction() = default;
  [[nodiscard]] virtual ValidationResult
  validate(const flecs::world &world) const = 0;
  virtual void execute(flecs::world &world) = 0;
};
