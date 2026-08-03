#!/usr/bin/env bash
set -euo pipefail
echo "==== TESTING NASA ===="
cd "$(dirname "$0")/.."
./test/build_and_run.sh test/main_test_nasa.cpp
