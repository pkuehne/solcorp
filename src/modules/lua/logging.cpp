#include "logging.h"
#include "spdlog/spdlog.h"
#include <sol/types.hpp>

void log_info(sol::this_state s, const std::string &message) {
  sol::state_view mod_state(s);
  std::string mod_name = mod_state["solcorp"]["mod_name"]();
  auto logger = spdlog::get(mod_name);
  if (logger) {
    logger->info("{}", message);
  }
}

void log_warn(sol::this_state s, const std::string &message) {
  sol::state_view mod_state(s);
  std::string mod_name = mod_state["solcorp"]["mod_name"]();
  auto logger = spdlog::get(mod_name);
  if (logger) {
    logger->warn("{}", message);
  }
}

void log_error(sol::this_state s, const std::string &message) {
  sol::state_view mod_state(s);
  std::string mod_name = mod_state["solcorp"]["mod_name"]();
  auto logger = spdlog::get(mod_name);
  if (logger) {
    logger->error("{}", message);
  }
}

void load_logging(sol::state &mod_state) {
  auto solcorp_ns = mod_state["solcorp"].get_or_create<sol::table>();
  auto logging_ns = solcorp_ns["logging"].get_or_create<sol::table>();

  std::string mod_name = solcorp_ns["mod_name"]();
  auto sink = spdlog::default_logger()->sinks()[0];
  auto logger = std::make_shared<spdlog::logger>(mod_name, sink);
  spdlog::register_logger(logger);

  logging_ns.set_function("info", log_info);
  logging_ns.set_function("warn", log_warn);
  logging_ns.set_function("error", log_error);
}
