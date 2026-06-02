#include "contract_actions.h"
#include "modules/base/notification.h"
#include "modules/engine/helpers.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <format>

namespace {
ValidationResult validateAcceptedContract(flecs::entity contract,
                                          const flecs::world &world) {
  if (!contract.is_valid()) {
    return ValidationResult::Fail("Contract is not valid");
  }
  if (!contract.has<Contract>()) {
    return ValidationResult::Fail("Entity is not a contract");
  }
  if (!contract.has<ContractCurrentState>(
          world.lookup("States::Contract::Accepted"))) {
    return ValidationResult::Fail("Contract is not accepted");
  }
  return ValidationResult::Pass();
}

} // namespace

ValidationResult
ContractAcceptAction::validate(const flecs::world &world) const {
  if (!contract.is_valid()) {
    return ValidationResult::Fail("Contract is not valid");
  }
  if (!contract.has<Contract>()) {
    return ValidationResult::Fail("Entity is not a contract");
  }
  if (!contract.has<ContractCurrentState>(
          world.lookup("States::Contract::Open"))) {
    return ValidationResult::Fail("Contract is not open");
  }
  return ValidationResult::Pass();
}

void ContractAcceptAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  contract.add<ContractCurrentState>(
      world.lookup("States::Contract::Accepted"));
  world.get_mut<Company>().balance += contract.get<Contract>().upfront_payment;
}

ValidationResult
ContractRejectAction::validate(const flecs::world &world) const {
  if (!contract.is_valid()) {
    return ValidationResult::Fail("Contract is not valid");
  }
  if (!contract.has<Contract>()) {
    return ValidationResult::Fail("Entity is not a contract");
  }
  if (contract.has<ContractCurrentState>(
          world.lookup("States::Contract::Closed"))) {
    return ValidationResult::Fail("Contract is already closed");
  }
  return ValidationResult::Pass();
}

void ContractRejectAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  auto &contractData = contract.get_mut<Contract>();
  if (contract.has<ContractCurrentState>(
          world.lookup("States::Contract::Accepted"))) {
    world.get_mut<Company>().balance -= contractData.upfront_payment;
  }
  contractData.failed = true;
  contract.add<ContractCurrentState>(world.lookup("States::Contract::Closed"));
}

ValidationResult
ContractSuccessAction::validate(const flecs::world &world) const {
  return validateAcceptedContract(contract, world);
}

void ContractSuccessAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  auto &contractData = contract.get_mut<Contract>();
  contractData.failed = false;
  world.get_mut<Company>().balance += contractData.completion_payment;

  instantiateNotification(
      world, "Contract Completed",
      std::format("'{}' has been completed and paid out ${}",
                  contractData.name.c_str(),
                  format_locale(contractData.completion_payment)),
      world.lookup("NotificationCategories::Contracts"),
      NotificationSeverity::Medium);
  contract.add<ContractCurrentState>(world.lookup("States::Contract::Closed"));
}

ValidationResult ContractFailAction::validate(const flecs::world &world) const {
  return validateAcceptedContract(contract, world);
}

void ContractFailAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  auto &contractData = contract.get_mut<Contract>();
  contractData.failed = true;

  instantiateNotification(
      world, "Contract Failed",
      std::format("'{}' has failed", contractData.name.c_str()),
      world.lookup("NotificationCategories::Contracts"),
      NotificationSeverity::Important);
  contract.add<ContractCurrentState>(world.lookup("States::Contract::Closed"));
}
