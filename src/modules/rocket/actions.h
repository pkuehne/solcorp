#pragma once

#include "../base/action.h"
#include <flecs.h>
#include <vector>

struct ScheduleLaunchAction : public IAction {
  int launchDay = 0;
  flecs::entity current = flecs::entity::null();
  std::string name;
  flecs::entity rocket = flecs::entity::null();
  flecs::entity launchpad = flecs::entity::null();
  flecs::entity targetOrbit =
      flecs::entity::null();           ///< Target orbit from CanLiftTo
  std::vector<flecs::entity> payloads; ///< Payloads to launch

  flecs::entity result = flecs::entity::null();

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &) override;
};

struct RocketEntity {
  flecs::entity value;
};
struct DestinationEntity {
  flecs::entity value;
};

struct MoveRocketAction : public IAction {
  flecs::entity rocket;
  flecs::entity destination;

  MoveRocketAction(RocketEntity r, DestinationEntity d)
      : rocket(r.value), destination(d.value) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};
