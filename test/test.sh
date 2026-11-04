#!/bin/sh
set -e
cd "$(dirname "$0")/.." || exit 1
./test/test_nasa.sh
./test/build_and_run.sh test/main_test_nasa_display.cpp
./test/test_non_nasa.sh
./test/test_non_nasa_queue.sh
