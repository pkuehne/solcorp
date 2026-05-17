#include "modules/engine/helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("format_locale", "[helpers]") {
  GIVEN("zero") {
    THEN("it formats as 0") { REQUIRE(format_locale(0) == "0"); }
  }

  GIVEN("a small positive amount") {
    THEN("it formats without commas") {
      REQUIRE(format_locale(5) == "5");
      REQUIRE(format_locale(42) == "42");
      REQUIRE(format_locale(999) == "999");
    }
  }

  GIVEN("a four-digit amount") {
    THEN("it inserts one comma") { REQUIRE(format_locale(1000) == "1,000"); }
  }

  GIVEN("a six-digit amount") {
    THEN("it inserts one comma") {
      REQUIRE(format_locale(999999) == "999,999");
    }
  }

  GIVEN("a seven-digit amount") {
    THEN("it inserts two commas") {
      REQUIRE(format_locale(1000000) == "1,000,000");
    }
  }

  GIVEN("a large amount") {
    THEN("it inserts commas every three digits") {
      REQUIRE(format_locale(1234567890) == "1,234,567,890");
    }
  }

  GIVEN("a negative amount") {
    THEN("it prefixes with -") {
      REQUIRE(format_locale(-1) == "-1");
      REQUIRE(format_locale(-999) == "-999");
      REQUIRE(format_locale(-1000) == "-1,000");
      REQUIRE(format_locale(-1234567) == "-1,234,567");
    }
  }
}
