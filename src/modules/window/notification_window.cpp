#include "notification_window.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include <flecs.h>
#include <algorithm>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

static ImVec4 severityColor(NotificationSeverity sev) {
  switch (sev) {
  case NotificationSeverity::Low:
    return {0.7f, 0.7f, 0.7f, 1.0f};
  case NotificationSeverity::Medium:
    return {1.0f, 0.85f, 0.2f, 1.0f};
  case NotificationSeverity::High:
    return {1.0f, 0.55f, 0.1f, 1.0f};
  case NotificationSeverity::Important:
    return {1.0f, 0.25f, 0.1f, 1.0f};
  case NotificationSeverity::Critical:
    return {1.0f, 0.05f, 0.05f, 1.0f};
  }
  return {1.0f, 1.0f, 1.0f, 1.0f};
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
  return true;
}

void showNotificationWindow(flecs::world &world) {
  showWindow(world, "Notifications");
}

void drawNotificationWindow(flecs::entity winE) {
  auto &state = winE.get_mut<NotificationWindow>();
  auto world = winE.world();

  // Guard against stale category filter entity
  if (state.category_filter.is_valid() && !state.category_filter.is_alive()) {
    state.category_filter = flecs::entity();
  }

  auto notif_query = world.query_builder<const Notification>().build();

  // Collect distinct categories from live notifications for the filter combo
  std::vector<flecs::entity> categories;
  notif_query.each([&](flecs::entity notifE, const Notification &) {
    auto cat = notifE.target<NotificationCategory>();
    if (!cat.is_valid()) {
      return;
    }
    for (const auto &c : categories) {
      if (c == cat) {
        return;
      }
    }
    categories.push_back(cat);
  });

  // --- Severity filter ---
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

  // --- Category filter ---
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

  // --- Mark All Read ---
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

  ImGui::Separator();

  // --- Notification list ---
  if (!ImGui::BeginChild("NotifList")) {
    ImGui::EndChild();
    return;
  }

  ImDrawList *dl = ImGui::GetWindowDrawList();
  bool any = false;
  flecs::entity clicked_notif = flecs::entity::null();

  std::vector<flecs::entity> visible;
  notif_query.each([&](flecs::entity notifE, const Notification &) {
    if (notificationMatchesFilter(notifE, state)) {
      visible.push_back(notifE);
    }
  });

  // Sort by entity ID (monotonically increasing = creation order) so that
  // archetype table moves (e.g. adding NotificationRead) don't disturb order.
  std::ranges::sort(visible,
                    [](flecs::entity a, flecs::entity b) { return a.id() < b.id(); });

  for (auto notifE : std::ranges::reverse_view(visible)) {
    const auto &notif = notifE.get<Notification>();
    any = true;

    ImGui::PushID(std::to_string(notifE.id()).c_str());
    float avail_w = ImGui::GetContentRegionAvail().x;

    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);

    ImGui::BeginGroup();
    ImGui::Dummy({0.0f, 2.0f});

    // Header: title + severity badge + category
    ImGui::TextUnformatted(notif.title.c_str());
    ImGui::SameLine();
    ImGui::TextColored(severityColor(notif.severity), "[%s]",
                       to_string(notif.severity));
    auto cat = notifE.target<NotificationCategory>();
    if (cat.is_valid()) {
      ImGui::SameLine();
      ImGui::Text("| %s", cat.name().c_str());
    }

    // Body text (wrapped)
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

    // Background drawn behind content via channel 0
    dl->ChannelsSetCurrent(0);
    if (is_unread) {
      dl->AddRectFilled(item_min, item_max, IM_COL32(60, 60, 130, 80), 4.0f);
    }
    if (hovered) {
      dl->AddRect(item_min, item_max, IM_COL32(200, 200, 200, 120), 4.0f);
    }
    dl->ChannelsMerge();

    if (ImGui::IsItemClicked()) {
      clicked_notif = notifE;
    }

    ImGui::Spacing();
    ImGui::PopID();
  }

  // Apply click-to-mark-read after iteration to avoid structural change during
  // each()
  if (clicked_notif.is_valid() && clicked_notif.is_alive()) {
    clicked_notif.add<NotificationRead>();
    spdlog::debug("Notification {} marked as read", clicked_notif.id());
  }

  if (!any) {
    ImGui::TextDisabled("No notifications");
  }

  ImGui::EndChild();
}
