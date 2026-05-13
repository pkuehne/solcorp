#pragma once

#include "../base/action.h"
#include <cstdint>
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

struct PrefabEntity {
  flecs::entity value;
};

struct LineEntity {
  flecs::entity value;
};

struct RocketBuildAction : public IAction {
  flecs::entity prefab = flecs::entity::null();
  flecs::entity line = flecs::entity::null();
  int64_t cost = 0;

  RocketBuildAction(PrefabEntity p, LineEntity l, int64_t c)
      : prefab(p.value), line(l.value), cost(c) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};

struct RocketCompleteBuildAction : public IAction {
  flecs::entity rocket = flecs::entity::null();

  explicit RocketCompleteBuildAction(flecs::entity r) : rocket(r) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
  void block(flecs::world &world) override;
};

struct RocketEntity {
  flecs::entity value;
};
struct DestinationEntity {
  flecs::entity value;
};

struct RocketMoveAction : public IAction {
  flecs::entity rocket;
  flecs::entity destination;
  uint8_t days = 0;

  RocketMoveAction(RocketEntity r, DestinationEntity d, uint8_t days)
      : rocket(r.value), destination(d.value), days(days) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};

struct RocketCompleteMoveAction : public IAction {
  flecs::entity rocket;

  explicit RocketCompleteMoveAction(flecs::entity r) : rocket(r) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
  void block(flecs::world &world) override;
};