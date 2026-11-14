#!/usr/bin/env bash
# Compile and run the frame model tests without ROS or Docker.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/dc2-model-test.XXXXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT

# Flags are conventional whitespace-separated compiler arguments, not shell code.
read -r -a flags <<< "${CXXFLAGS:-}"
"${CXX:-c++}" "${flags[@]}" -std=c++17 -UNDEBUG -I"$root/include" \
  "$root/test/frame_model_test.cpp" "$root/src/frame_model.cpp" \
  -o "$build_dir/frame_model_test"
"$build_dir/frame_model_test"
