#include "notification_window.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include <algorithm>
#include <flecs.h>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

static ImColor severityColor(NotificationSeverity sev) {
  switch (sev) {
  case NotificationSeverity::Low:
    return {178, 178, 178};
  case NotificationSeverity::Medium:
    return {255, 217, 51};
  case NotificationSeverity::High:
    return {255, 140, 26};
  case NotificationSeverity::Important:
    return {255, 64, 26};
  case NotificationSeverity::Critical:
    return {255, 13, 13};
  }
  return {255, 255, 255};
}

static bool drawNotification(flecs::entity notifE, ImDrawList *dl) {
  const auto &notif = notifE.get<Notification>();

  ImGui::PushID(std::to_string(notifE.id()).c_str());
  float avail_w = ImGui::GetContentRegionAvail().x;

  dl->ChannelsSplit(2);
  dl->ChannelsSetCurrent(1);

  ImGui::BeginGroup();
  ImGui::Dummy({0.0f, 2.0f});

  ImGui::TextUnformatted(notif.title.c_str());
  ImGui::SameLine();
  ImGui::TextColored(severityColor(notif.severity), "[%s]",
                     to_string(notif.severity));
  auto cat = notifE.target<NotificationCategory>();
  if (cat.is_valid()) {
    ImGui::SameLine();
    ImGui::Text("| %s", cat.name().c_str());
  }

  ImGui::Indent(8.0f);
  ImGui::PushTextWrapPos(0.0f);
  ImGui::TextUnformatted(notif.text.c_str());
  ImGui::PopTextWrapPos();
  ImGui::Unindent(8.0f);

  ImGui::Dummy({0.0f, 2.0f});
  ImGui::EndGroup();

  ImVec2 item_min = ImGui::GetItemRectMin();
  ImVec2 item_max = {item_min.x + avail_w, ImGui::GetItemRectMax().y};

  bool is_unread = !notifE.has<NotificationRead>();
  bool hovered = ImGui::IsItemHovered();
  bool clicked = ImGui::IsItemClicked();

  dl->ChannelsSetCurrent(0);
  if (is_unread) {
    dl->AddRectFilled(item_min, item_max, IM_COL32(60, 60, 130, 80), 4.0f);
  }
  if (hovered) {
    dl->AddRect(item_min, item_max, IM_COL32(200, 200, 200, 120), 4.0f);
  }
  dl->ChannelsMerge();

  ImGui::Spacing();
  ImGui::PopID();

  return clicked;
}

bool notificationMatchesFilter(flecs::entity notifE,
                               const NotificationWindow &state) {
  const auto *notif = notifE.try_get<Notification>();
  if (!notif) {
    return false;
  }
  if (state.severity_filter >= 0 &&
      notif->severity !=
          static_cast<NotificationSeverity>(state.severity_filter)) {
    return false;
  }
  if (state.category_filter.is_valid()) {
    auto cat = notifE.target<NotificationCategory>();
    if (cat != state.category_filter) {
      return false;
    }
  }
  if (state.unread_only && notifE.has<NotificationRead>()) {
    return false;
  }
  return true;
}

static void drawFilterRow(NotificationWindow &state,
                          const std::vector<flecs::entity> &categories,
                          const flecs::query<const Notification> &notif_query) {
  const char *sev_label =
      state.severity_filter < 0
          ? "All Severities"
          : to_string(static_cast<NotificationSeverity>(state.severity_filter));
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("##SevFilter", sev_label)) {
    if (ImGui::Selectable("All Severities", state.severity_filter < 0)) {
      state.severity_filter = -1;
    }
    for (auto sev :
         {NotificationSeverity::Low, NotificationSeverity::Medium,
          NotificationSeverity::High, NotificationSeverity::Important,
          NotificationSeverity::Critical}) {
      bool sel = state.severity_filter == static_cast<int>(sev);
      if (ImGui::Selectable(to_string(sev), sel)) {
        state.severity_filter = static_cast<int>(sev);
      }
    }
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  const char *cat_label = state.category_filter.is_valid()
                              ? state.category_filter.name().c_str()
                              : "All Categories";
  ImGui::SetNextItemWidth(160.0f);
  if (ImGui::BeginCombo("##CatFilter", cat_label)) {
    if (ImGui::Selectable("All Categories",
                          !state.category_filter.is_valid())) {
      state.category_filter = flecs::entity();
    }
    for (auto &cat : categories) {
      bool sel = (state.category_filter == cat);
      if (ImGui::Selectable(cat.name().c_str(), sel)) {
        state.category_filter = cat;
      }
    }
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  ImGui::Checkbox("Unread Only", &state.unread_only);

  ImGui::SameLine();
  if (ImGui::Button("Mark All Read")) {
    std::vector<flecs::entity> to_mark;
    notif_query.each([&](flecs::entity notifE, const Notification &) {
      if (notificationMatchesFilter(notifE, state)) {
        to_mark.push_back(notifE);
      }
    });
    for (auto &e : to_mark) {
      e.add<NotificationRead>();
    }
  }
}

void showNotificationWindow(flecs::world &world) {
  showWindow(world, "Notifications");
}

void drawNotificationWindow(flecs::entity winE) {
  auto &state = winE.get_mut<NotificationWindow>();
  auto world = winE.world();

  auto notif_query = world.query_builder<const Notification>().build();

  std::vector<flecs::entity> categories;
  auto cat_node = world.lookup("NotificationCategories");
  if (cat_node.is_valid()) {
    cat_node.children([&](flecs::entity e) { categories.push_back(e); });
  }

  drawFilterRow(state, categories, notif_query);

  ImGui::Separator();

  // --- Notification list ---
  if (!ImGui::BeginChild("NotifList")) {
    ImGui::EndChild();
    return;
  }

  std::vector<flecs::entity> notifications;
  notif_query.each([&](flecs::entity notifE, const Notification &) {
    if (notificationMatchesFilter(notifE, state)) {
      notifications.push_back(notifE);
    }
  });

  std::ranges::sort(notifications, [](flecs::entity a, flecs::entity b) {
    return a.get<Notification>().id < b.get<Notification>().id;
  });

  ImDrawList *dl = ImGui::GetWindowDrawList();
  flecs::entity clicked_notif = flecs::entity::null();
  for (auto notifE : std::ranges::reverse_view(notifications)) {
    if (drawNotification(notifE, dl)) {
      clicked_notif = notifE;
    }
  }

  // Apply click-to-mark-read after iteration to avoid structural change during
  // each()
  if (clicked_notif.is_valid()) {
    clicked_notif.add<NotificationRead>();
  }

  if (notifications.empty()) {
    ImGui::TextDisabled("No notifications");
  }

  ImGui::EndChild();
}
