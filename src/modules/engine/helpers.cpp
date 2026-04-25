#include "helpers.h"
#include <random>

bool roll_random(double probability) {
  static std::mt19937 rng{std::random_device{}()};
  static std::uniform_real_distribution<double> dist{0.0, 1.0};
  return dist(rng) < probability;
}
