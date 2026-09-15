#!/usr/bin/env bash
set -e

DHU_BIN="${DHU_PATH:-$HOME/Library/Android/sdk/extras/google/auto/desktop-head-unit}"
if [ ! -f "$DHU_BIN" ] && [ -n "$ANDROID_HOME" ]; then
    DHU_BIN="$ANDROID_HOME/extras/google/auto/desktop-head-unit"
fi

echo "=== Launching Android Auto Desktop Head Unit (USB mode) ==="
echo "Ensuring clean ADB server state..."
adb kill-server 2>/dev/null || true

if [ -f "$DHU_BIN" ]; then
    echo "Starting DHU binary at $DHU_BIN..."
    "$DHU_BIN" --usb
else
    echo "Warning: desktop-head-unit not found at standard path."
    echo "Please set DHU_PATH or install via SDK Manager: sdkmanager 'extras;google;auto'"
    exit 1
fi
