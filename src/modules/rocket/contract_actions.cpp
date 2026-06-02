#include "contract_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>

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

  contract.add<ContractCurrentState>(world.lookup("States::Contract::Accepted"));
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
ContractCompleteAction::validate(const flecs::world &world) const {
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

void ContractCompleteAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  auto &contractData = contract.get_mut<Contract>();
  contractData.failed = failed;
  if (!failed) {
    world.get_mut<Company>().balance += contractData.completion_payment;
  }
  contract.add<ContractCurrentState>(world.lookup("States::Contract::Closed"));
}
