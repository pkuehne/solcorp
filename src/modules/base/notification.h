#pragma once
#include <flecs.h>
#include <string>

/// @brief Severity levels for notifications, used to determine how they are
/// displayed to the player.
enum class NotificationSeverity : uint8_t {
  Low,
  Medium,
  High,
  Important,
  Critical
};

/// @brief Component representing a notification to be shown to the player.
struct Notification {
  std::string title;
  std::string text;
  NotificationSeverity severity = NotificationSeverity::Low;
};

/// @brief Exclusive relationship. Target entity is the category (a child of
/// the NotificationCategories node). Mods can add categories by creating new
/// entities under NotificationCategories.
struct NotificationCategory {};

/// @brief Tag to indicate a notification has been read by the player.
struct NotificationRead {};

std::string_view to_string(NotificationSeverity severity);

void systemCreateNotificationNodes(flecs::iter &it);

void registerNotificationComponents(flecs::world &world);
void registerNotificationSystems(flecs::world &world);

flecs::entity instantiateNotification(
    flecs::world &world, const std::string &title, const std::string &text,
    flecs::entity category = {},
    NotificationSeverity severity = NotificationSeverity::Low);
