#!/usr/bin/env bash
set -e

PORT="${1:-/dev/ttyUSB0}"
BAUD="${2:-115200}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FIRMWARE_DIR="$REPO_ROOT/firmware"

echo "=== Flashing Erebus ESP32-S3 N16R8 on $PORT ($BAUD bauds) ==="

if [ -f "/opt/esp-idf/export.sh" ]; then
    . /opt/esp-idf/export.sh > /dev/null 2>&1
elif [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/export.sh" ]; then
    . "$IDF_PATH/export.sh" > /dev/null 2>&1
fi

cd "$FIRMWARE_DIR"
idf.py -p "$PORT" -b "$BAUD" flash
