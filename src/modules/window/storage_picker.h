#pragma once
#include <flecs.h>
#include <functional>

void storagePickerPopup(const char *popupId, bool open, flecs::world &world,
                        flecs::entity excluded,
                        const std::function<void(flecs::entity)> &onConfirm);
