#pragma once

#include "../base/action.h"
#include <flecs.h>

struct ContractAcceptAction : public IAction {
  flecs::entity contract = flecs::entity::null();

  ContractAcceptAction() = default;
  explicit ContractAcceptAction(flecs::entity c) : contract(c) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};

struct ContractRejectAction : public IAction {
  flecs::entity contract = flecs::entity::null();

  ContractRejectAction() = default;
  explicit ContractRejectAction(flecs::entity c) : contract(c) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};

struct ContractSuccessAction : public IAction {
  flecs::entity contract = flecs::entity::null();

  ContractSuccessAction() = default;
  explicit ContractSuccessAction(flecs::entity c) : contract(c) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};

struct ContractFailAction : public IAction {
  flecs::entity contract = flecs::entity::null();

  ContractFailAction() = default;
  explicit ContractFailAction(flecs::entity c) : contract(c) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};
