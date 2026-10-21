#!/usr/bin/env bash
set -euo pipefail
mkdir -p reports
exec > >(tee reports/install.log) 2>&1
arduino-cli version
index=https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index --additional-urls "$index"
arduino-cli core install esp32:esp32@3.3.2 --additional-urls "$index"
arduino-cli lib install 'ArduinoJson@7.4.2' 'GFX Library for Arduino@1.6.9' 'XPT2046_Touchscreen@1.4'
arduino-cli core list
arduino-cli lib list
