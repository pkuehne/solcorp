#define CATCH_CONFIG_RUNNER
#include "spdlog/cfg/env.h"
#include "spdlog/details/log_msg_buffer.h"
#include "spdlog/sinks/ansicolor_sink.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/spdlog.h"
#include <catch2/catch_all.hpp>
#include <iostream>
#include <mutex>
#include <vector>

class buffered_sink : public spdlog::sinks::sink {
public:
  void log(const spdlog::details::log_msg &msg) override {
    std::scoped_lock lock(mutex_);
    buffer_.emplace_back(msg);
  }

  void flush() override {}
  void set_pattern(const std::string &) override {}
  void set_formatter(std::unique_ptr<spdlog::formatter>) override {}

  void clear() {
    std::scoped_lock lock(mutex_);
    buffer_.clear();
  }

  void dump_to_stderr() {
    std::scoped_lock lock(mutex_);
    spdlog::sinks::ansicolor_stderr_sink_st color_sink(
        spdlog::color_mode::always);
    for (auto const &msg : buffer_) {
      color_sink.log(msg);
    }
    color_sink.flush();
  }

private:
  std::mutex mutex_;
  std::vector<spdlog::details::log_msg_buffer> buffer_;
};

static std::shared_ptr<buffered_sink> g_log_sink;

struct SpdlogCapture : Catch::EventListenerBase {
  using EventListenerBase::EventListenerBase;

  void testCaseStarting(Catch::TestCaseInfo const &) override {
    if (g_log_sink) {
      g_log_sink->clear();
    }
  }

  void testCaseEnded(Catch::TestCaseStats const &stats) override {
    if (g_log_sink && stats.totals.assertions.failed > 0) {
      std::cerr << "---- spdlog output ----\n";
      g_log_sink->dump_to_stderr();
      std::cerr << "-----------------------\n";
    }
  }
};

CATCH_REGISTER_LISTENER(SpdlogCapture)

int main(int argc, char **argv) {
  g_log_sink = std::make_shared<buffered_sink>();
  auto logger = std::make_shared<spdlog::logger>("", g_log_sink);
  spdlog::set_default_logger(logger);
  spdlog::cfg::load_env_levels();
  if (spdlog::default_logger()->level() > spdlog::level::debug) {
    spdlog::set_level(spdlog::level::debug);
  }
  return Catch::Session().run(argc, argv);
}
