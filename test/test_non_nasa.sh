#!/bin/sh
echo ==== TESTING Non NASA ====
cd "$(dirname "$0")/.." || exit 1
./test/build_and_run.sh test/main_test_non_nasa.cpp