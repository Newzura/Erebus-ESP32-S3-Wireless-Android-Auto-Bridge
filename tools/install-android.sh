#!/usr/bin/env bash
set -e

APK_PATH="app/build/outputs/apk/debug/app-debug.apk"
if [ ! -f "$APK_PATH" ]; then
    APK_PATH="android-app/app/build/outputs/apk/debug/app-debug.apk"
fi

if [ ! -f "$APK_PATH" ]; then
    echo "Error: APK not found at $APK_PATH. Please run ./tools/build-android.sh first."
    exit 1
fi

echo "Installing $APK_PATH on target Android device..."
adb install -r "$APK_PATH"
