#include "modules/window/notification_window.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Notification window filtering", "[notification_window]") {
  flecs::world world;
  world.import <BaseModule>();

  auto catA = createNotificationCategory(world, "CategoryA");
  auto catB = createNotificationCategory(world, "CategoryB");

  auto lowA = instantiateNotification(world, "Low A", "text", catA,
                                      NotificationSeverity::Low);
  auto highA = instantiateNotification(world, "High A", "text", catA,
                                       NotificationSeverity::High);
  auto lowB = instantiateNotification(world, "Low B", "text", catB,
                                      NotificationSeverity::Low);
  auto critB = instantiateNotification(world, "Crit B", "text", catB,
                                       NotificationSeverity::Critical);

  auto notif_query = world.query_builder<const Notification>().build();

  GIVEN("No filters active") {
    NotificationWindow state{};

    WHEN("checking if a notification matches") {
      THEN("all notifications match") {
        REQUIRE(notificationMatchesFilter(lowA, state));
        REQUIRE(notificationMatchesFilter(highA, state));
        REQUIRE(notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }

    WHEN("marking all filtered notifications read") {
      markFilteredNotificationsRead(notif_query, state);

      THEN("all notifications are marked read") {
        REQUIRE(lowA.has<NotificationRead>());
        REQUIRE(highA.has<NotificationRead>());
        REQUIRE(lowB.has<NotificationRead>());
        REQUIRE(critB.has<NotificationRead>());
      }
    }
  }

  GIVEN("Severity filter set to High") {
    NotificationWindow state{.severity_filter =
                                 static_cast<int>(NotificationSeverity::High)};

    WHEN("checking if a notification matches") {
      THEN("only High severity notifications match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
        REQUIRE(notificationMatchesFilter(highA, state));
        REQUIRE(!notificationMatchesFilter(lowB, state));
        REQUIRE(!notificationMatchesFilter(critB, state));
      }
    }

    WHEN("marking all filtered notifications read") {
      markFilteredNotificationsRead(notif_query, state);

      THEN("only High severity notifications are marked read") {
        REQUIRE(!lowA.has<NotificationRead>());
        REQUIRE(highA.has<NotificationRead>());
        REQUIRE(!lowB.has<NotificationRead>());
        REQUIRE(!critB.has<NotificationRead>());
      }
    }
  }

  GIVEN("Category filter set to CategoryB") {
    NotificationWindow state{.category_filter = catB};

    WHEN("checking if a notification matches") {
      THEN("only CategoryB notifications match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
        REQUIRE(!notificationMatchesFilter(highA, state));
        REQUIRE(notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }

    WHEN("marking all filtered notifications read") {
      markFilteredNotificationsRead(notif_query, state);

      THEN("only CategoryB notifications are marked read") {
        REQUIRE(!lowA.has<NotificationRead>());
        REQUIRE(!highA.has<NotificationRead>());
        REQUIRE(lowB.has<NotificationRead>());
        REQUIRE(critB.has<NotificationRead>());
      }
    }
  }

  GIVEN("Both severity Critical and category B filters active") {
    NotificationWindow state{
        .severity_filter = static_cast<int>(NotificationSeverity::Critical),
        .category_filter = catB};

    WHEN("checking if a notification matches") {
      THEN("only Critical + CategoryB notifications match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
        REQUIRE(!notificationMatchesFilter(highA, state));
        REQUIRE(!notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }
  }

  GIVEN("Unread Only filter active") {
    NotificationWindow state{.unread_only = true};

    WHEN("no notifications have been read") {
      THEN("all notifications match") {
        REQUIRE(notificationMatchesFilter(lowA, state));
        REQUIRE(notificationMatchesFilter(highA, state));
        REQUIRE(notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }

    WHEN("a notification is marked read") {
      lowA.add<NotificationRead>();

      THEN("the read notification does not match") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
      }

      THEN("unread notifications still match") {
        REQUIRE(notificationMatchesFilter(highA, state));
        REQUIRE(notificationMatchesFilter(lowB, state));
        REQUIRE(notificationMatchesFilter(critB, state));
      }
    }

    WHEN("marking all filtered notifications read") {
      lowA.add<NotificationRead>();
      markFilteredNotificationsRead(notif_query, state);

      THEN("unread ones are marked read and already-read ones are unaffected") {
        REQUIRE(lowA.has<NotificationRead>());
        REQUIRE(highA.has<NotificationRead>());
        REQUIRE(lowB.has<NotificationRead>());
        REQUIRE(critB.has<NotificationRead>());
      }
    }
  }

  GIVEN("Unread Only filter combined with severity filter") {
    NotificationWindow state{.severity_filter =
                                 static_cast<int>(NotificationSeverity::Low),
                             .unread_only = true};

    WHEN("a matching notification is marked read") {
      lowA.add<NotificationRead>();

      THEN("it no longer matches") {
        REQUIRE(!notificationMatchesFilter(lowA, state));
      }

      THEN("unread Low notifications still match") {
        REQUIRE(notificationMatchesFilter(lowB, state));
      }
    }
  }

  GIVEN("An entity without a Notification component") {
    auto plainEntity = world.entity("Plain");
    NotificationWindow state{};

    WHEN("checking if it matches") {
      THEN("it does not match") {
        REQUIRE(!notificationMatchesFilter(plainEntity, state));
      }
    }
  }
}
