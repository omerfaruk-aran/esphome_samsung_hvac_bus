#!/bin/sh
echo ==== TESTING Non NASA queue ====
cd "$(dirname "$0")/.." || exit 1
./test/build_and_run.sh test/main_test_non_nasa_queue.cpp
