#pragma once

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