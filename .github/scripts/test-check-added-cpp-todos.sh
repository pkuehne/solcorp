#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
check_script="$script_dir/check-added-cpp-todos.sh"

tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT

cd "$tmp_dir"
git init -q
git config user.name tester
git config user.email tester@example.com

assert_check_passes() {
  local description=$1

  if ! bash "$check_script" "$base_sha" "$head_sha"; then
    echo "Expected check to pass: $description" >&2
    exit 1
  fi
}

assert_check_fails() {
  local description=$1

  if bash "$check_script" "$base_sha" "$head_sha"; then
    echo "Expected check to fail: $description" >&2
    exit 1
  fi
}

write_and_commit() {
  local path=$1
  local content=$2
  mkdir -p "$(dirname "$path")"
  printf '%s' "$content" > "$path"
  git add "$path"
  git commit -q -m "update $path"
  head_sha=$(git rev-parse HEAD)
}

write_and_commit "src/example.cpp" $'int main() {\n  return 0;\n}\n'
base_sha=$head_sha

write_and_commit "src/example.cpp" $'int main() {\n  // TODO: follow up\n  return 0;\n}\n'
assert_check_fails "TODO in added cpp line comment"
base_sha=$head_sha

write_and_commit "src/example.cpp" $'int main() {\n  // DONE: follow up\n  return 0;\n}\n'
assert_check_passes "non-TODO line comment in cpp file"
base_sha=$head_sha

write_and_commit "src/example.cpp" $'int main() {\n  // TODOABLE marker\n  return 0;\n}\n'
assert_check_passes "TODO as part of a longer word in cpp line comment"
base_sha=$head_sha

write_and_commit "src/example.cpp" $'int main() {\n  /* TODO: follow up */\n  return 0;\n}\n'
assert_check_passes "TODO in non-line cpp comment"
base_sha=$head_sha

write_and_commit "src/example.h" $'// TODO: follow up\n#pragma once\n'
assert_check_fails "TODO in added header line comment"
base_sha=$head_sha

write_and_commit "scripts/example.lua" $'-- TODO: follow up\n'
assert_check_passes "TODO outside cpp and header files"
