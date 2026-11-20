#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${GFX_DIR:?Adafruit GFX directory}"
mkdir -p build
g++ -std=c++17 -DARDUINO=10819 -I tests/host -I "$GFX_DIR" tools/thumbs.cpp RundeUhr/qrcodegen.c "$GFX_DIR/Adafruit_GFX.cpp" -o build/thumbs
build/thumbs
python3 tools/thumbs_to_json.py
python3 tools/build_web.py
