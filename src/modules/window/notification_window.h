#pragma once

#include <flecs.h>
#include <modules/base/notification.h>

struct NotificationWindow {
  int severity_filter = -1; // -1 = All; otherwise cast to NotificationSeverity
  flecs::entity category_filter = {};
  bool unread_only = false;
};

void showNotificationWindow(flecs::world &world);
void drawNotificationWindow(flecs::entity winE);

/// @brief Returns true if the notification passes the active filters.
bool notificationMatchesFilter(flecs::entity notifE,
                                const NotificationWindow &state);

/// @brief Adds NotificationRead to every notification that passes the active
/// filters.
void markFilteredNotificationsRead(
    const flecs::query<const Notification> &notif_query,
    const NotificationWindow &state);
