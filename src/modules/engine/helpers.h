#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <string>

/// @brief Returns true with the given probability.
/// @param probability Chance of returning true, in the range [0.0, 1.0].
/// @return true if the roll succeeds, false otherwise.
bool roll_random(double probability);

/// @brief Formats an amount with commas as thousands separators, and a minus
/// sign if negative.
/// @details Despite the name, is not locale-aware and always uses commas and a
/// minus sign.
/// @param amount The amount of money.
/// @return A string representation of the amount.
std::string format_locale(int64_t amount);

template <std::floating_point T> std::string format_locale(T amount) {
  return format_locale(static_cast<int64_t>(std::llround(amount)));
}