#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${GFX_DIR:?Adafruit GFX folder}"
g++ -std=c++17 -DARDUINO=10819 -I tests/host -I RundeUhr -I "$GFX_DIR" tests/layout_test.cpp "$GFX_DIR/Adafruit_GFX.cpp" RundeUhr/qrcodegen.c -o build/layout_test
build/layout_test
