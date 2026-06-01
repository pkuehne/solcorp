#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("Stats", "[stats]") {
  GIVEN("A test stat with value 10") {
    Stat stat{{.id = "test_stat",
               .display = "Displ",
               .description = "More Text",
               .base = 10.0}};

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

SCENARIO("Stat formatting", "[stats]") {
  GIVEN("a default number stat") {
    Stat stat{{.id = "count",
               .display = "Count",
               .description = "A plain number",
               .base = 12.4}};

    THEN("it formats as a rounded number") {
      REQUIRE(stat.format(12.4) == "12");
    }
  }

  GIVEN("a currency stat") {
    Stat stat{{.id = "cost",
               .display = "Cost",
               .description = "A money value",
               .base = 1'250,
               .format = Stat::Format::Currency}};

    THEN("it formats with a dollar prefix and separators") {
      REQUIRE(stat.format(1250) == "$1,250");
    }
  }

  GIVEN("a percentage stat") {
    Stat stat{{.id = "failure-rate",
               .display = "Failure Rate",
               .description = "A probability",
               .base = 0.1,
               .format = Stat::Format::Percentage}};

    THEN("it formats probabilities as whole percentages") {
      REQUIRE(stat.format(stat.base()) == "10%");
    }
  }
}
