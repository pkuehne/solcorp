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

void registerNotificationSystems(flecs::world &world) {
  world.system("Create Notification Nodes")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemCreateNotificationNodes);
}

std::string_view to_string(NotificationSeverity severity) {
  switch (severity) {
  case NotificationSeverity::Low:
    return "Low";
  case NotificationSeverity::Medium:
    return "Medium";
  case NotificationSeverity::High:
    return "High";
  case NotificationSeverity::Important:
    return "Important";
  case NotificationSeverity::Critical:
    return "Critical";
  }
  return "Unknown";
}

void createNotificationNodes(flecs::world &world) {
  world.entity("Notifications");
  auto categories = world.entity("NotificationCategories");
  world.entity("NotificationCategories::None").child_of(categories);
}

void systemCreateNotificationNodes(flecs::iter &it) {
  auto world = it.world();
  createNotificationNodes(world);
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

  category = category.is_valid() ? category
                                 : world.entity("NotificationCategories::None");
  e.add<NotificationCategory>(category);

  spdlog::debug("Notification: {} - {} ({} @ {})", title, text,
                category.name().c_str(), to_string(severity));
  return e;
}
