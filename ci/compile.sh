#!/usr/bin/env bash
set -euo pipefail
mkdir -p reports/firmware
cp firmware/TramliBern/*.ino firmware/TramliBern/*.h reports/firmware/
fqbn='esp32:esp32:esp32:CPUFreq=240,FlashSize=4M,PartitionScheme=default,PSRAM=disabled'
printf '%s\n' "$fqbn" > reports/board.txt
sha256sum firmware/TramliBern/TramliBern.ino > reports/sketch.sha256
set +e
arduino-cli compile --fqbn "$fqbn" --warnings default --output-dir reports/firmware firmware/TramliBern 2>&1 | tee reports/compile.log
status=${PIPESTATUS[0]}
set -e
python3 ci/memory_report.py "$status"
if [ "$status" -eq 0 ]; then python3 ci/package_firmware.py; fi
exit "$status"
