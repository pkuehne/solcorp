#pragma once

#include <flecs.h>
#include <string>

struct ValidationResult {
  bool ok = false;
  std::string message;

  static ValidationResult Pass() { return {true, {}}; }
  static ValidationResult Fail(const std::string &msg) {
    return {false, std::move(msg)};
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
  virtual ValidationResult validate(const flecs::world &world) const = 0;
  virtual void execute(flecs::world &world) = 0;
};

struct PlannedLaunch {
  int launchDay = 0;
  flecs::entity current = flecs::entity::null();
  std::string name;
  flecs::entity rocket = flecs::entity::null();
  flecs::entity launchpad = flecs::entity::null();

  flecs::entity result = flecs::entity::null();

  virtual ValidationResult validate(const flecs::world &world) const;
  virtual void execute(flecs::world &);
};
