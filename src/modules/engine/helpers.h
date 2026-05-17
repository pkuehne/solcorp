#pragma once

#include <cmath>
#include <cstdint>
#include <string>

/// @brief Returns true with the given probability.
/// @param probability Chance of returning true, in the range [0.0, 1.0].
/// @return true if the roll succeeds, false otherwise.
bool roll_random(double probability);

/// @brief Formats a monetary amount as a string.
/// @param amount The amount of money.
/// @return A string representation of the amount.
std::string format_money(int64_t amount);

template <std::floating_point T> std::string format_money(T amount) {
  return format_money(static_cast<int64_t>(std::llround(amount)));
}