#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("Stats", "[stats]") {
  GIVEN("A test stat with value 10") {
    Stat stat = Stat("test_stat", "Displ", "More Text", 10.0);

    THEN("The base value is 10") { REQUIRE(stat.base() == 10.0); }

    WHEN("A multiplier mod of 2 is applied") {
      Modifier mod = {
          .target_stat = "test_stat", .additive = 0.0, .multiplicative = 2.0};
      stat.addModifier(mod, "");
      THEN("The result is 20") { REQUIRE(stat.value() == 20.0); }
    }

    WHEN("An adder mod of 10 is applied") {
      Modifier mod = {
          .target_stat = "test_stat", .additive = 10.0, .multiplicative = 1.0};
      stat.addModifier(mod, "");
      THEN("The result is 20") { REQUIRE(stat.value() == 20.0); }
    }

    WHEN("An adder mod of 10 is applied and then reset") {
      Modifier mod = {
          .target_stat = "test_stat", .additive = 10.0, .multiplicative = 1.0};
      bool result = stat.addModifier(mod, "");
      stat.reset();

      THEN("The result is again 10") {
        REQUIRE(result == true);
        REQUIRE(stat.value() == 10.0);
      }
    }

    WHEN("An unrelated modifier is added") {
      Modifier mod = {
          .target_stat = "other_stat", .additive = 10.0, .multiplicative = 1.0};
      bool result = stat.addModifier(mod, "");
      THEN("The result is again 10") {
        REQUIRE(result == false);
        REQUIRE(stat.value() == 10.0);
      }
    }
  }
}
