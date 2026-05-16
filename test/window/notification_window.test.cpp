#include "modules/window/notification_window.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("notificationMatchesFilter", "[notification_window]") {
  flecs::world world;
  world.import<BaseModule>();

  auto catA = createNotificationCategory(world, "CategoryA");
  auto catB = createNotificationCategory(world, "CategoryB");

  auto lowA =
      instantiateNotification(world, "Low A", "text", catA, NotificationSeverity::Low);
  auto highA =
      instantiateNotification(world, "High A", "text", catA, NotificationSeverity::High);
  auto lowB =
      instantiateNotification(world, "Low B", "text", catB, NotificationSeverity::Low);
  auto critB =
      instantiateNotification(world, "Crit B", "text", catB, NotificationSeverity::Critical);

  GIVEN("No filters active") {
    NotificationWindow state{};

    WHEN("Any notification is checked") {
      THEN("All notifications match") {
        REQUIRE(notificationMatchesFilter(lowA, state));
        REQUIRE(notificationMatchesFilter(highA, state));
        REQUIRE(notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }
  }

  GIVEN("Severity filter set to High") {
    NotificationWindow state{.severity_filter =
                                 static_cast<int>(NotificationSeverity::High)};

    WHEN("Notifications are checked") {
      THEN("Only High severity notifications match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
        REQUIRE(notificationMatchesFilter(highA, state));
        REQUIRE(!notificationMatchesFilter(lowB, state));
        REQUIRE(!notificationMatchesFilter(critB, state));
      }
    }
  }

  GIVEN("Category filter set to CategoryB") {
    NotificationWindow state{.category_filter = catB};

    WHEN("Notifications are checked") {
      THEN("Only CategoryB notifications match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
        REQUIRE(!notificationMatchesFilter(highA, state));
        REQUIRE(notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }
  }

  GIVEN("Both severity Critical and category B filters active") {
    NotificationWindow state{
        .severity_filter = static_cast<int>(NotificationSeverity::Critical),
        .category_filter = catB};

    WHEN("Notifications are checked") {
      THEN("Only Critical + CategoryB notifications match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
        REQUIRE(!notificationMatchesFilter(highA, state));
        REQUIRE(!notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }
  }

  GIVEN("Unread Only filter active") {
    NotificationWindow state{.unread_only = true};

    WHEN("A notification has not been read") {
      THEN("It matches") {
        REQUIRE(notificationMatchesFilter(lowA, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }

    WHEN("A notification is marked read") {
      lowA.add<NotificationRead>();

      THEN("The read notification does not match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
      }

      THEN("Unread notifications still match") {
        REQUIRE(notificationMatchesFilter(highA, state));
        REQUIRE(notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }
  }

  GIVEN("Unread Only filter combined with severity filter") {
    NotificationWindow state{.severity_filter =
                                 static_cast<int>(NotificationSeverity::Low),
                             .unread_only = true};

    WHEN("A matching notification is marked read") {
      lowA.add<NotificationRead>();

      THEN("It no longer matches") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
      }

      THEN("Unread Low notifications still match") {
        REQUIRE(notificationMatchesFilter(lowB, state));
      }
    }
  }

  GIVEN("An entity without a Notification component") {
    auto plainEntity = world.entity("Plain");
    NotificationWindow state{};

    WHEN("It is checked against filters") {
      THEN("It does not match") {
        REQUIRE(!notificationMatchesFilter(plainEntity, state));
      }
    }
  }
}
