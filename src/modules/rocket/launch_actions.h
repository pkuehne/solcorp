#pragma once

#include "../base/action.h"
#include <flecs.h>
#include <vector>

struct ScheduleLaunchAction : public IAction {
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

struct EditLaunchAction : public IAction {
  flecs::entity plan = flecs::entity::null();
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

struct CancelLaunchAction : public IAction {
  flecs::entity plan = flecs::entity::null();

  CancelLaunchAction() = default;
  explicit CancelLaunchAction(flecs::entity p) : plan(p) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};
