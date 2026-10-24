#!/usr/bin/env bash
set -euo pipefail
mkdir -p reports
fqbn='esp32:esp32:esp32:CPUFreq=240,FlashSize=4M,PartitionScheme=default,PSRAM=disabled'
printf '%s\n' "$fqbn" > reports/board.txt
sha256sum firmware/TramliBern/TramliBern.ino > reports/sketch.sha256
set +e
arduino-cli compile --fqbn "$fqbn" --warnings default firmware/TramliBern 2>&1 | tee reports/compile.log
status=${PIPESTATUS[0]}
set -e
python3 ci/memory_report.py "$status"
exit "$status"
