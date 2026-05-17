#include "modules/engine/helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("format_money", "[helpers]") {
  GIVEN("zero") {
    THEN("it formats as $0") { REQUIRE(format_money(0) == "$0"); }
  }

  GIVEN("a small positive amount") {
    THEN("it formats without commas") {
      REQUIRE(format_money(5) == "$5");
      REQUIRE(format_money(42) == "$42");
      REQUIRE(format_money(999) == "$999");
    }
  }

  GIVEN("a four-digit amount") {
    THEN("it inserts one comma") { REQUIRE(format_money(1000) == "$1,000"); }
  }

  GIVEN("a six-digit amount") {
    THEN("it inserts one comma") {
      REQUIRE(format_money(999999) == "$999,999");
    }
  }

  GIVEN("a seven-digit amount") {
    THEN("it inserts two commas") {
      REQUIRE(format_money(1000000) == "$1,000,000");
    }
  }

  GIVEN("a large amount") {
    THEN("it inserts commas every three digits") {
      REQUIRE(format_money(1234567890) == "$1,234,567,890");
    }
  }

  GIVEN("a negative amount") {
    THEN("it prefixes with $-") {
      REQUIRE(format_money(-1) == "$-1");
      REQUIRE(format_money(-999) == "$-999");
      REQUIRE(format_money(-1000) == "$-1,000");
      REQUIRE(format_money(-1234567) == "$-1,234,567");
    }
  }
}
