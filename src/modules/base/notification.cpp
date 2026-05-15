#include "notification.h"
#include "modules/lua/lua.h"
#include <spdlog/spdlog.h>

void registerNotificationComponents(flecs::world &world) {
  world.component<NotificationSeverity>()
      .constant("Low", NotificationSeverity::Low)
      .constant("Medium", NotificationSeverity::Medium)
      .constant("High", NotificationSeverity::High)
      .constant("Important", NotificationSeverity::Important)
      .constant("Critical", NotificationSeverity::Critical);
  world.component<NotificationCategory>().add(flecs::Exclusive);
  world.component<NotificationRead>();
  world.component<Notification>()
      .member("title", &Notification::title)
      .member("text", &Notification::text)
      .member("severity", &Notification::severity);
  register_enum_table_lua(world, "NotificationSeverity", [](LuaEnumBuilder &b) {
    b.value("Low", NotificationSeverity::Low)
        .value("Medium", NotificationSeverity::Medium)
        .value("High", NotificationSeverity::High)
        .value("Important", NotificationSeverity::Important)
        .value("Critical", NotificationSeverity::Critical);
  });
}

void systemCreateNotificationNodes(flecs::iter &it) {
  auto world = it.world();
  world.entity("Notifications");
  world.entity("NotificationCategories");
}

void registerNotificationSystems(flecs::world &world) {
  world.system("Create Notification Nodes")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemCreateNotificationNodes);
}

flecs::entity instantiateNotification(flecs::world &world,
                                      const std::string &title,
                                      const std::string &text,
                                      flecs::entity category,
                                      NotificationSeverity severity) {
  auto notificationsNode = world.lookup("Notifications");
  auto e = world.entity()
               .set<Notification>(
                   {.title = title, .text = text, .severity = severity})
               .child_of(notificationsNode);
  if (category.is_valid()) {
    e.add<NotificationCategory>(category);
  }
  spdlog::debug("New notification: {} - {}", title, text);
  return e;
}
