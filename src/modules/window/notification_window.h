#pragma once

#include <flecs.h>
#include <modules/base/notification.h>

struct NotificationWindow {
  int severity_filter = -1; // -1 = All; otherwise cast to NotificationSeverity
  flecs::entity category_filter = {};
};

void showNotificationWindow(flecs::world &world);
void drawNotificationWindow(flecs::entity winE);

/// @brief Returns true if the notification passes the active filters.
bool notificationMatchesFilter(flecs::entity notifE,
                                const NotificationWindow &state);
