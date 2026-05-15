#include "modules/base/notification.h"
#include "modules/base/base.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("instantiateNotification", "[base][notification]") {
  flecs::world world;
  world.import <BaseModule>();
  GIVEN("A valid category and explicit severity") {
    auto categories = world.entity("NotificationCategories");
    auto category =
        world.entity("NotificationCategories::Custom").child_of(categories);

    WHEN("A notification is created") {
      auto e = instantiateNotification(world, "Launch Ready", "Rocket is ready",
                                       category, NotificationSeverity::High);

      THEN("The entity is valid and parented to Notifications") {
        CHECK(e.is_valid());
        CHECK(e.parent() == world.lookup("Notifications"));
      }

      THEN("The Notification component carries the given title, text, and "
           "severity") {
        const auto &n = e.get<Notification>();
        CHECK(n.title == "Launch Ready");
        CHECK(n.text == "Rocket is ready");
        CHECK(n.severity == NotificationSeverity::High);
      }

      THEN("The entity has the given category relationship") {
        CHECK(e.has<NotificationCategory>(category));
      }
    }
  }

  GIVEN("No category (invalid entity)") {
    WHEN("A notification is created") {
      auto e = instantiateNotification(world, "Alert", "FYI");

      THEN("NotificationCategories::None is assigned") {
        auto none = world.lookup("NotificationCategories::None");
        CHECK(e.has<NotificationCategory>(none));
      }

      THEN("Severity defaults to Low") {
        CHECK(e.get<Notification>().severity == NotificationSeverity::Low);
      }
    }
  }
}
