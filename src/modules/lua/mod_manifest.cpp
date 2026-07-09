#include "mod_manifest.h"
#include "lua_data.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>

bool operator>=(const ModVersion &lhs, const ModVersion &rhs) {
  if (lhs.major != rhs.major) {
    return lhs.major > rhs.major;
  }
  if (lhs.minor != rhs.minor) {
    return lhs.minor > rhs.minor;
  }
  return lhs.patch >= rhs.patch;
}

ModVersion parseModVersion(const std::string &version) {
  // Lenient parse of "major.minor.patch": missing or non-numeric components
  // simply coerce to 0.
  ModVersion result;
  size_t start = 0;
  for (int *component : {&result.major, &result.minor, &result.patch}) {
    size_t dot = version.find('.', start);
    *component = std::atoi(version.substr(start, dot - start).c_str());
    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }
  return result;
}

bool versionAtLeast(const ModVersion &have, const std::string &min_version) {
  if (min_version.empty()) {
    return true;
  }
  return have >= parseModVersion(min_version);
}

namespace {

// Follow the dependency edges among `unresolved` nodes to find one concrete
// cycle, returned as a path like {"a", "b", "a"} for the error message.
std::vector<std::string>
findCycle(const std::map<std::string, std::vector<std::string>> &deps,
          const std::set<std::string> &unresolved) {
  enum Color : std::uint8_t { White, Gray, Black };
  std::map<std::string, Color> color;
  for (const auto &id : unresolved) {
    color[id] = White;
  }

  std::vector<std::string> stack;
  std::vector<std::string> cycle;

  std::function<bool(const std::string &)> visit =
      [&](const std::string &node) -> bool {
    color[node] = Gray;
    stack.push_back(node);
    auto it = deps.find(node);
    if (it != deps.end()) {
      for (const auto &next : it->second) {
        if (unresolved.find(next) == unresolved.end()) {
          continue;
        }
        if (color[next] == Gray) {
          auto start = std::ranges::find(stack, next);
          cycle.assign(start, stack.end());
          cycle.push_back(next);
          return true;
        }
        if (color[next] == White && visit(next)) {
          return true;
        }
      }
    }
    stack.pop_back();
    color[node] = Black;
    return false;
  };

  for (const auto &id : unresolved) {
    if (color[id] == White && visit(id)) {
      break;
    }
  }
  return cycle;
}

} // namespace

std::vector<std::string>
resolveLoadOrder(const std::vector<ModManifest> &mods) {
  std::map<std::string, const ModManifest *> by_id;
  for (const auto &mod : mods) {
    if (!by_id.emplace(mod.id, &mod).second) {
      throw ModDependencyError("duplicate mod id '" + mod.id + "'");
    }
  }

  // Validate dependencies and build the graph (dependency -> dependents).
  std::map<std::string, std::vector<std::string>> dependencies; // mod -> deps
  std::map<std::string, std::vector<std::string>> dependents;   // dep -> mods
  std::map<std::string, int> in_degree;
  for (const auto &mod : mods) {
    in_degree[mod.id]; // ensure present
  }

  for (const auto &mod : mods) {
    for (const auto &dep : mod.dependencies) {
      auto it = by_id.find(dep.id);
      if (it == by_id.end()) {
        throw ModDependencyError("mod '" + mod.id +
                                 "' depends on unknown mod '" + dep.id + "'");
      }
      if (!versionAtLeast(parseModVersion(it->second->version),
                          dep.min_version)) {
        throw ModDependencyError("mod '" + mod.id + "' requires '" + dep.id +
                                 "' >= " + dep.min_version + " but found " +
                                 it->second->version);
      }
      dependencies[mod.id].push_back(dep.id);
      dependents[dep.id].push_back(mod.id);
      in_degree[mod.id]++;
    }
  }

  // Kahn's algorithm; ready nodes processed in id order for determinism.
  std::set<std::string> ready;
  for (const auto &[id, degree] : in_degree) {
    if (degree == 0) {
      ready.insert(id);
    }
  }

  std::vector<std::string> order;
  while (!ready.empty()) {
    std::string id = *ready.begin();
    ready.erase(ready.begin());
    order.push_back(id);
    auto it = dependents.find(id);
    if (it != dependents.end()) {
      for (const auto &dependent : it->second) {
        if (--in_degree[dependent] == 0) {
          ready.insert(dependent);
        }
      }
    }
  }

  if (order.size() != mods.size()) {
    std::set<std::string> unresolved;
    for (const auto &[id, degree] : in_degree) {
      if (std::ranges::find(order, id) == order.end()) {
        unresolved.insert(id);
      }
    }
    std::vector<std::string> cycle = findCycle(dependencies, unresolved);
    std::string path;
    for (size_t i = 0; i < cycle.size(); ++i) {
      path += cycle[i];
      if (i + 1 < cycle.size()) {
        path += " -> ";
      }
    }
    throw ModDependencyError("dependency cycle detected: " + path);
  }

  return order;
}

ModManifest readModManifest(const std::filesystem::path &dir) {
  std::filesystem::path file = dir / "mod.lua";
  LuaDataFile data(file.string());
  if (!data.ok()) {
    throw ModDependencyError("mod '" + dir.filename().string() +
                             "': " + data.error());
  }

  ModManifest manifest;
  manifest.id = dir.filename().string();
  LuaTableView root = data.root();
  manifest.name = root.getString("name").value_or(manifest.id);
  manifest.version = root.getString("version").value_or("0.0.0");
  manifest.description = root.getString("description").value_or("");
  manifest.author = root.getString("author").value_or("");
  manifest.dev_only = root.getBool("dev_only").value_or(false);

  root.forEachArrayElement("dependencies", [&](const LuaValue &value) {
    ModDependency dep;
    if (auto id = value.asString()) {
      dep.id = *id;
    } else if (auto table = value.asTable()) {
      dep.id = table->getString("id").value_or("");
      dep.min_version = table->getString("version").value_or("");
    }
    if (!dep.id.empty()) {
      manifest.dependencies.push_back(dep);
    }
  });

  return manifest;
}
