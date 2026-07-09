#!/usr/bin/env bash
#
# Select and clang-tidy a set of C++ translation units.
#
# Usage:
#   lint-cpp.sh                 lint .cpp files with uncommitted changes
#                               (staged, unstaged, or untracked) — fast local default
#   lint-cpp.sh --since REF     lint .cpp files this branch changed vs REF (REF...HEAD)
#   lint-cpp.sh --all           lint every .cpp under src/ and test/ (full sweep)
#   lint-cpp.sh ... --list      print the selected files and exit (no clang-tidy)
#
# clang-tidy only lints translation units (.cpp). A header-only change is covered
# when an including .cpp is in the same change set; otherwise the --all sweep
# (run by the merge/main CI) catches it.
#
# Environment:
#   BUILD_DIR   directory holding compile_commands.json (default: build)
#
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"

usage() {
  cat <<'EOF'
Usage:
  lint-cpp.sh                 lint .cpp files with uncommitted changes (default)
  lint-cpp.sh --since REF     lint .cpp files this branch changed vs REF (REF...HEAD)
  lint-cpp.sh --all           lint every .cpp under src/ and test/ (full sweep)
  lint-cpp.sh ... --list      print the selected files and exit (no clang-tidy)

Environment: BUILD_DIR   directory with compile_commands.json (default: build)
EOF
}

mode="changed"
base=""
list_only=0

while [ $# -gt 0 ]; do
  case "$1" in
    --all) mode="all" ;;
    --since)
      mode="since"
      base="${2:-}"
      [ -n "$base" ] || { echo "error: --since requires a git ref" >&2; exit 2; }
      shift
      ;;
    --list) list_only=1 ;;
    -h | --help) usage; exit 0 ;;
    *) echo "error: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

# Print the newline-separated .cpp files selected for the current mode. Kept as a
# single pure function so the selection can be exercised via --list in tests.
select_files() {
  case "$mode" in
    all)
      find src test -name '*.cpp'
      ;;
    since)
      git diff --name-only --diff-filter=d "${base}...HEAD" -- src test | grep -E '\.cpp$' || true
      ;;
    changed)
      {
        git diff --name-only --diff-filter=d -- src test
        git diff --name-only --diff-filter=d --cached -- src test
        git ls-files --others --exclude-standard -- src test
      } | grep -E '\.cpp$' || true
      ;;
  esac
}

mapfile -t files < <(select_files | sort -u)

if [ "$list_only" -eq 1 ]; then
  if [ "${#files[@]}" -gt 0 ]; then
    printf '%s\n' "${files[@]}"
  fi
  exit 0
fi

if [ "${#files[@]}" -eq 0 ]; then
  echo "clang-tidy: no ${mode} .cpp files to lint"
  exit 0
fi

echo "clang-tidy: linting ${#files[@]} file(s) [${mode}]:"
printf '  %s\n' "${files[@]}"

printf '%s\0' "${files[@]}" |
  xargs -0 -r -n 1 -P "$(nproc)" clang-tidy --quiet -p "$BUILD_DIR"
