#define CATCH_CONFIG_RUNNER
#include "spdlog/cfg/env.h"
#include "spdlog/spdlog.h"
#include <catch2/catch_all.hpp>

int main(int argc, char **argv) {
  spdlog::cfg::load_env_levels();
  if (spdlog::default_logger()->level() > spdlog::level::debug)
    spdlog::set_level(spdlog::level::debug);
  return Catch::Session().run(argc, argv);
}
