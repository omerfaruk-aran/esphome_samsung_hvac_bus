#!/bin/sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
g++ -std=c++17 \
  "$ROOT/test/main_test_power_allocator.cpp" \
  "$ROOT/components/samsung_ac/power_allocator.cpp" \
  -I"$ROOT/test" \
  -I"$ROOT/components" \
  -o "$ROOT/test/test_power_allocator.exe"
chmod +x "$ROOT/test/test_power_allocator.exe"
"$ROOT/test/test_power_allocator.exe"
