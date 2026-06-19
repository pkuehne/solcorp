#include "modules/lua/mod_manifest.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("versionAtLeast compares a version against a minimum requirement") {
  GIVEN("a version of 1.2.3") {
    ModVersion have = parseModVersion("1.2.3");

    WHEN("the minimum is an empty string") {
      THEN("any version satisfies it") { REQUIRE(versionAtLeast(have, "")); }
    }

    WHEN("the minimum equals the version") {
      THEN("it is satisfied") { REQUIRE(versionAtLeast(have, "1.2.3")); }
    }

    WHEN("the minimum is older") {
      THEN("it is satisfied across each component") {
        REQUIRE(versionAtLeast(have, "1.2.2"));
        REQUIRE(versionAtLeast(have, "1.1.9"));
        REQUIRE(versionAtLeast(have, "0.9.9"));
      }
    }

    WHEN("the minimum is newer") {
      THEN("it is not satisfied across each component") {
        REQUIRE_FALSE(versionAtLeast(have, "1.2.4"));
        REQUIRE_FALSE(versionAtLeast(have, "1.3.0"));
        REQUIRE_FALSE(versionAtLeast(have, "2.0.0"));
      }
    }
  }

  GIVEN("versions with omitted trailing components") {
    THEN("missing components default to zero") {
      REQUIRE(versionAtLeast(parseModVersion("2"), "2.0.0"));
      REQUIRE(versionAtLeast(parseModVersion("2.1"), "2.1.0"));
      REQUIRE_FALSE(versionAtLeast(parseModVersion("2"), "2.0.1"));
    }
  }

  GIVEN("a non-numeric version string") {
    THEN("non-numeric components coerce to zero rather than throwing") {
      REQUIRE(versionAtLeast(parseModVersion("1.x.0"), "1.0.0"));
      REQUIRE_FALSE(versionAtLeast(parseModVersion("abc"), "0.0.1"));
    }
  }
}
