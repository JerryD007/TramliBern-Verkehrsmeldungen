#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${GFX_DIR:?Set Adafruit GFX directory}"
mkdir -p build/fire_frames
g++ -std=c++17 -DARDUINO=10819 -I tests/host -I "$GFX_DIR" experiments/feuerwehr/test.cpp "$GFX_DIR/Adafruit_GFX.cpp" -o build/fire_test
build/fire_test
