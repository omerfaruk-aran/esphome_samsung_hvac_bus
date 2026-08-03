#!/usr/bin/env bash
# Run every native protocol test. Exits non-zero if any suite fails.
set -euo pipefail
cd "$(dirname "$0")/.."
./test/test_nasa.sh
./test/test_non_nasa.sh
echo "==== ALL TESTS PASSED ===="
