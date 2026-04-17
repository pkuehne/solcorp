#include "modules/base/base.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

struct Marker {};
struct OtherMarker {};

SCENARIO("findAncestorWith", "[base][util]") {
  flecs::world world;

  GIVEN("An entity that itself has the component") {
    auto e = world.entity().add<Marker>();

    THEN("It returns itself") { CHECK(findAncestorWith<Marker>(e) == e); }
  }

  GIVEN("An entity whose direct parent has the component") {
    auto parent = world.entity().add<Marker>();
    auto child = world.entity().child_of(parent);

    THEN("It returns the parent") {
      CHECK(findAncestorWith<Marker>(child) == parent);
    }
  }

  GIVEN("An entity whose grandparent has the component") {
    auto grandparent = world.entity().add<Marker>();
    auto parent = world.entity().child_of(grandparent);
    auto child = world.entity().child_of(parent);

    THEN("It returns the grandparent") {
      CHECK(findAncestorWith<Marker>(child) == grandparent);
    }
  }

  GIVEN("No ancestor has the component") {
    auto parent = world.entity();
    auto child = world.entity().child_of(parent);

    THEN("It returns a null entity") {
      CHECK(!findAncestorWith<Marker>(child).is_valid());
    }
  }

  GIVEN("A root entity with no parent and no component") {
    auto e = world.entity();

    THEN("It returns a null entity") {
      CHECK(!findAncestorWith<Marker>(e).is_valid());
    }
  }

  GIVEN("The nearest ancestor with the component is not the furthest one") {
    auto grandparent = world.entity().add<Marker>();
    auto parent = world.entity().add<Marker>().child_of(grandparent);
    auto child = world.entity().child_of(parent);

    THEN("It returns the closer ancestor") {
      CHECK(findAncestorWith<Marker>(child) == parent);
    }
  }

  GIVEN("An ancestor with a different component") {
    auto parent = world.entity().add<OtherMarker>();
    auto child = world.entity().child_of(parent);

    THEN("It returns a null entity") {
      CHECK(!findAncestorWith<Marker>(child).is_valid());
    }
  }

  GIVEN("A destroyed entity") {
    auto e = world.entity().add<Marker>();
    e.destruct();

    THEN("It returns a null entity") {
      CHECK(!findAncestorWith<Marker>(e).is_valid());
    }
  }
}
