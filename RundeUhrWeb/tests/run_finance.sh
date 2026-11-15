#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${GFX_DIR:?Adafruit GFX folder}"
: "${JSON_DIR:?ArduinoJson src folder}"
mkdir -p build preview
g++ -std=c++17 -DARDUINO=10819 -DARDUINOJSON_ENABLE_ARDUINO_STRING=0 -DARDUINOJSON_ENABLE_ARDUINO_STREAM=0 -DARDUINOJSON_ENABLE_ARDUINO_PRINT=0 -DARDUINOJSON_ENABLE_PROGMEM=0 -ffunction-sections -fdata-sections -I tests/host -I "$GFX_DIR" -I "$JSON_DIR" tests/finance_test.cpp "$GFX_DIR/Adafruit_GFX.cpp" -Wl,--gc-sections -o build/finance
build/finance
