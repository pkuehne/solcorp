#include "modules/lua/mod_manifest.h"
#include <catch2/catch_test_macros.hpp>

namespace {

ModManifest mod(const std::string &id, const std::string &version,
                std::vector<ModDependency> deps = {}) {
  ModManifest m;
  m.id = id;
  m.name = id;
  m.version = version;
  m.dependencies = std::move(deps);
  return m;
}

// Index of an id within an order vector (for "comes before" assertions).
long indexOf(const std::vector<std::string> &order, const std::string &id) {
  for (size_t i = 0; i < order.size(); ++i) {
    if (order[i] == id) {
      return static_cast<long>(i);
    }
  }
  return -1;
}

} // namespace

SCENARIO("resolveLoadOrder topologically orders mods by dependency") {
  GIVEN("a linear chain c -> b -> a") {
    std::vector<ModManifest> mods = {
        mod("c", "1.0.0", {{"b", ""}}),
        mod("b", "1.0.0", {{"a", ""}}),
        mod("a", "1.0.0"),
    };
    WHEN("the load order is resolved") {
      auto order = resolveLoadOrder(mods);
      THEN("dependencies come before their dependents") {
        REQUIRE(order.size() == 3);
        REQUIRE(indexOf(order, "a") < indexOf(order, "b"));
        REQUIRE(indexOf(order, "b") < indexOf(order, "c"));
      }
    }
  }

  GIVEN("a diamond a -> {b, c} -> d") {
    std::vector<ModManifest> mods = {
        mod("a", "1.0.0", {{"b", ""}, {"c", ""}}),
        mod("b", "1.0.0", {{"d", ""}}),
        mod("c", "1.0.0", {{"d", ""}}),
        mod("d", "1.0.0"),
    };
    WHEN("the load order is resolved") {
      auto order = resolveLoadOrder(mods);
      THEN("d loads first and a loads last") {
        REQUIRE(order.front() == "d");
        REQUIRE(order.back() == "a");
        REQUIRE(indexOf(order, "d") < indexOf(order, "b"));
        REQUIRE(indexOf(order, "d") < indexOf(order, "c"));
      }
    }
  }

  GIVEN("independent mods given in arbitrary discovery order") {
    std::vector<ModManifest> mods = {
        mod("zeta", "1.0.0"),
        mod("alpha", "1.0.0"),
        mod("mike", "1.0.0"),
    };
    WHEN("resolved") {
      auto order = resolveLoadOrder(mods);
      THEN("ties break deterministically by id") {
        REQUIRE(order == std::vector<std::string>{"alpha", "mike", "zeta"});
      }
    }
  }

  GIVEN("a mod depending on an unknown mod") {
    std::vector<ModManifest> mods = {mod("a", "1.0.0", {{"ghost", ""}})};
    THEN("resolution throws ModDependencyError") {
      REQUIRE_THROWS_AS(resolveLoadOrder(mods), ModDependencyError);
    }
  }

  GIVEN("a dependency whose version is too old") {
    std::vector<ModManifest> mods = {
        mod("a", "1.0.0", {{"b", "2.0.0"}}),
        mod("b", "1.5.0"),
    };
    THEN("resolution throws ModDependencyError") {
      REQUIRE_THROWS_AS(resolveLoadOrder(mods), ModDependencyError);
    }
  }

  GIVEN("a dependency whose version is satisfied") {
    std::vector<ModManifest> mods = {
        mod("a", "1.0.0", {{"b", "1.5.0"}}),
        mod("b", "1.5.0"),
    };
    THEN("resolution succeeds") { REQUIRE_NOTHROW(resolveLoadOrder(mods)); }
  }

  GIVEN("a dependency cycle a -> b -> a") {
    std::vector<ModManifest> mods = {
        mod("a", "1.0.0", {{"b", ""}}),
        mod("b", "1.0.0", {{"a", ""}}),
    };
    THEN("resolution throws ModDependencyError") {
      REQUIRE_THROWS_AS(resolveLoadOrder(mods), ModDependencyError);
    }
  }

  GIVEN("two mods sharing the same id") {
    std::vector<ModManifest> mods = {
        mod("dup", "1.0.0"),
        mod("dup", "2.0.0"),
    };
    THEN("resolution throws ModDependencyError") {
      REQUIRE_THROWS_AS(resolveLoadOrder(mods), ModDependencyError);
    }
  }
}
