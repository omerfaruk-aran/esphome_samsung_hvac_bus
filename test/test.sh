#!/bin/sh
cd "$(dirname "$0")/.." || exit 1
./test/test_nasa.sh
./test/test_non_nasa.sh