#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FIRMWARE_DIR="$REPO_ROOT/firmware"

echo "=== Building Erebus ESP32-S3 N16R8 Firmware ==="

if [ -f "/opt/esp-idf/export.sh" ]; then
    echo "Sourcing ESP-IDF environment from /opt/esp-idf/export.sh..."
    . /opt/esp-idf/export.sh > /dev/null 2>&1
elif [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/export.sh" ]; then
    echo "Sourcing ESP-IDF from \$IDF_PATH..."
    . "$IDF_PATH/export.sh" > /dev/null 2>&1
fi

cd "$FIRMWARE_DIR"

if [ ! -f "sdkconfig" ]; then
    echo "Setting target to esp32s3..."
    idf.py set-target esp32s3
fi

echo "Compiling firmware with idf.py build..."
idf.py build

echo "=== Firmware Build Complete ==="
echo "Binary: $FIRMWARE_DIR/build/erebus-firmware.bin"
