#include "storage_picker.h"
#include "imgui.h"
#include "modules/site/site.h"
#include "widgets/widgets.h"
#include <flecs.h>

void storagePickerPopup(const char *popupId, bool open, flecs::world &world,
                        flecs::entity excluded,
                        const std::function<void(flecs::entity)> &onConfirm) {
  static flecs::entity destination;
  static std::string display = "<Select one>";
  if (open) {
    destination = flecs::entity();
    display = "<Select one>";
    ImGui::OpenPopup(popupId);
  }
  if (!ImGui::BeginPopupModal(popupId)) {
    return;
  }
  ImGui::Text("Where to?");
  ImGui::SameLine();

  auto storageFacilities =
      world.query_builder().with<Storage>().with<Site>().up().build();

  if (ImGui::BeginCombo("##StorageCombo", display.c_str())) {
    storageFacilities.each([&](flecs::entity s) {
      ImGui::BeginDisabled(excluded.is_valid() && s == excluded);
      if (ImGui::Selectable(s.parent().name().c_str(), destination == s)) {
        destination = s;
        display = s.parent().name();
      }
      ImGui::EndDisabled();
    });
    ImGui::EndCombo();
  }
  ImGui::Separator();
  if (ImGui::Button("Cancel")) {
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  std::string issue;
  if (!destination.is_valid()) {
    issue = "Select a destination";
  } else if (destination == excluded) {
    issue = "Already here";
  }
  if (Widgets::ActionButton(ButtonLabel{.text = "Ok"},
                            ButtonTooltip{.text = "Confirm selection"},
                            issue)) {
    onConfirm(destination);
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
}
