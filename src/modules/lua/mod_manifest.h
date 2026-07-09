/**
 * @file mod_manifest.h
 * @brief Mod manifest model and dependency-resolution logic.
 *
 * @details
 * Each mod directory declares a manifest in `mod.lua` (a file that returns a
 * table of name/version/dependencies). The engine reads every manifest up
 * front, then resolves a deterministic load order via topological sort,
 * validating minimum dependency versions. Missing dependencies, too-old
 * versions, and dependency cycles are unrecoverable and surface as
 * ModDependencyError.
 *
 * The graph/version functions take no lua_State and are unit-tested directly;
 * only readModManifest() touches Lua (via LuaDataFile).
 */
#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

/** @brief A parsed semantic version (major.minor.patch). */
struct ModVersion {
  int major = 0;
  int minor = 0;
  int patch = 0;
};

/** @brief Lexicographic >= over (major, minor, patch). */
bool operator>=(const ModVersion &lhs, const ModVersion &rhs);

/** @brief A dependency on another mod, with an optional minimum version. */
struct ModDependency {
  std::string id;          // referenced mod's directory id
  std::string min_version; // "" = any version
};

/** @brief One mod's manifest, keyed by its directory id. */
struct ModManifest {
  std::string id;          // directory name (canonical key)
  std::string name;        // display name
  std::string version;     // e.g. "0.1.0"
  std::string description; // optional human-readable summary
  std::string author;      // optional author / attribution
  bool dev_only = false;   // excluded from the release load order (ADR 011 §6)
  std::vector<ModDependency> dependencies;
};

/** @brief Unrecoverable mod manifest / dependency problem. */
class ModDependencyError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

/**
 * @brief Parse a "major.minor.patch" string. Missing or non-numeric components
 * coerce to 0 (e.g. "2" -> 2.0.0, "1.x" -> 1.0.0).
 */
ModVersion parseModVersion(const std::string &version);

/**
 * @brief Whether `have` satisfies a minimum-version requirement.
 * @param min_version Minimum required version; empty means "any" (always true).
 */
bool versionAtLeast(const ModVersion &have, const std::string &min_version);

/**
 * @brief Resolve mod load order from manifests.
 * @return Mod ids in load order (dependencies first), ties broken by id.
 * @throws ModDependencyError on a duplicate id, an unknown dependency, a
 * dependency whose version is too old, or a dependency cycle.
 */
std::vector<std::string> resolveLoadOrder(const std::vector<ModManifest> &mods);

/**
 * @brief Read `<dir>/mod.lua` into a ModManifest (id = directory name).
 * @throws ModDependencyError if the file is missing or does not return a table.
 */
ModManifest readModManifest(const std::filesystem::path &dir);
