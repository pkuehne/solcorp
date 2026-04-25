#pragma once

/// @brief Returns true with the given probability.
/// @param probability Chance of returning true, in the range [0.0, 1.0].
/// @return true if the roll succeeds, false otherwise.
bool roll_random(double probability);
