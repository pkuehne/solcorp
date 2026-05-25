#pragma once

#include "../base/action.h"
#include <flecs.h>
#include <vector>

struct LaunchScheduleAction : public IAction {
  int launchDay = 0;
  std::string name;
  flecs::entity rocket = flecs::entity::null();
  flecs::entity launchpad = flecs::entity::null();
  flecs::entity targetOrbit = flecs::entity::null();
  std::vector<flecs::entity> payloads;

  flecs::entity result = flecs::entity::null();

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};

struct LaunchCancelAction : public IAction {
  flecs::entity plan = flecs::entity::null();

  LaunchCancelAction() = default;
  explicit LaunchCancelAction(flecs::entity p) : plan(p) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};

struct LaunchInitiateRolloutAction : public IAction {
  flecs::entity plan = flecs::entity::null();

  LaunchInitiateRolloutAction() = default;
  explicit LaunchInitiateRolloutAction(flecs::entity p) : plan(p) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};

struct LaunchCompleteRolloutAction : public IAction {
  flecs::entity plan = flecs::entity::null();

  LaunchCompleteRolloutAction() = default;
  explicit LaunchCompleteRolloutAction(flecs::entity p) : plan(p) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};

struct LaunchGoAction : public IAction {
  flecs::entity plan = flecs::entity::null();

  LaunchGoAction() = default;
  explicit LaunchGoAction(flecs::entity p) : plan(p) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};
