#!/usr/bin/env bash
# Build one native test translation unit against the protocol sources and run it.
# Usage: test/build_and_run.sh test/main_test_nasa.cpp
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_UNDER_TEST="$1"

CXX="${CXX:-g++}"
CXXFLAGS=(-std=c++17 -Wall)
COMPONENT="$ROOT/components/samsung_ac"
SOURCES=(
  "$COMPONENT/protocol.cpp"
  "$COMPONENT/protocol_nasa.cpp"
  "$COMPONENT/protocol_non_nasa.cpp"
  "$COMPONENT/util.cpp"
  "$COMPONENT/debug_mqtt.cpp"
  "$COMPONENT/log.cpp"
)

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT
BIN="$WORKDIR/samsung_ac_test"

"$CXX" "${CXXFLAGS[@]}" -I"$ROOT/test" \
  "$ROOT/$SRC_UNDER_TEST" "${SOURCES[@]}" \
  -o "$BIN"

"$BIN"
