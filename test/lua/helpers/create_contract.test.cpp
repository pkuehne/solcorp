#include "modules/lua/helpers.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_contract", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();
  world.entity("Contracts");

  GIVEN("a contract name") {
    WHEN("create_contract is called") {
      auto contract = create_contract(world, "Mission1", "AcmeCorp",
                                      "Deliver satellite", 2000.0f, 8000.0f);
      THEN("a valid entity is returned") { REQUIRE(contract.is_valid()); }
      THEN("the Contract component has the correct fields") {
        auto &c = contract.get<Contract>();
        CHECK(c.name == "Mission1");
        CHECK(c.client == "AcmeCorp");
        CHECK(c.description == "Deliver satellite");
        CHECK(c.upfront_payment == 2000.0f);
        CHECK(c.completion_payment == 8000.0f);
      }
      THEN("the entity is a child of the Contracts node") {
        CHECK(contract.parent() == world.lookup("Contracts"));
      }
    }
  }

  GIVEN("a name that is already used by another contract") {
    auto first =
        create_contract(world, "Mission1", "OldClient", "OldDesc", 0, 0);
    WHEN("create_contract is called with the same name") {
      auto second =
          create_contract(world, "Mission1", "NewClient", "NewDesc", 0, 0);
      THEN("a new valid entity is returned") {
        REQUIRE(second.is_valid());
        CHECK(second != first);
      }
      THEN("the new contract stores the name in the Contract component") {
        CHECK(second.get<Contract>().name == "Mission1");
      }
    }
  }
}
