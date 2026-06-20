#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

namespace solcorp::test {

// Writes Lua source to a uniquely-named temp file and removes it on scope exit,
// so each test exercises the real on-disk load path of LuaDataFile.
class TempLuaFile {
public:
  explicit TempLuaFile(const std::string &contents) {
    static std::atomic<int> counter{0};
    path_ = std::filesystem::temp_directory_path() /
            ("solcorp_data_test_" + std::to_string(counter++) + ".lua");
    std::ofstream(path_) << contents;
  }
  ~TempLuaFile() {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }

  TempLuaFile(const TempLuaFile &) = delete;
  TempLuaFile &operator=(const TempLuaFile &) = delete;

  [[nodiscard]] std::string path() const { return path_.string(); }

private:
  std::filesystem::path path_;
};

} // namespace solcorp::test
