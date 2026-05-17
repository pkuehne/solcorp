#include "helpers.h"
#include <cstddef>
#include <random>

bool roll_random(double probability) {
  static std::mt19937 rng{std::random_device{}()};
  static std::uniform_real_distribution<double> dist{0.0, 1.0};
  return dist(rng) < probability;
}

std::string format_locale(int64_t amount) {
  std::string result = "";
  if (amount < 0) {
    result += "-";
    amount = -amount;
  }
  auto abs_amount = static_cast<uint64_t>(amount);
  std::string digits = std::to_string(abs_amount);
  auto insert_position = static_cast<std::ptrdiff_t>(digits.length()) - 3;
  while (insert_position > 0) {
    digits.insert(static_cast<std::string::size_type>(insert_position), ",");
    insert_position -= 3;
  }
  result += digits;
  return result;
}
