#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <base-sha> <head-sha>" >&2
  exit 2
fi

base_sha=$1
head_sha=$2
# Keep the marker split so this check does not match its own source.
marker='T''O''D''O'

matches=$(
  git diff --unified=0 --no-color --diff-filter=ACMR "$base_sha" "$head_sha" -- \
    | awk '
        BEGIN { file = "<unknown>"; hunk = "" }
        /^\+\+\+ b\// { sub(/^\+\+\+ b\//, "", $0); file = $0; next }
        /^@@ / { hunk = $0; next }
        /^\+/ && !/^\+\+\+/ { print file "\t" hunk "\t" substr($0, 2) }
      ' \
    | grep -F "$marker" || true
)

if [[ -n "$matches" ]]; then
  echo "Found disallowed task markers in added lines:"
  printf '%s\n' "$matches"
  exit 1
fi

echo "No disallowed task markers found in added lines."
