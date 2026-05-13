#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <base-sha> <head-sha>" >&2
  exit 2
fi

base_sha=$1
head_sha=$2

if git diff --unified=0 --no-color "$base_sha" "$head_sha" -- ':(glob)**/*.h' ':(glob)**/*.cpp' \
  | grep -E '^\+.*//.*\<TODO\>'; then
  echo "Found TODO comments in added C++ lines. Please track follow-up work in an issue instead of adding bare TODO markers."
  exit 1
fi
