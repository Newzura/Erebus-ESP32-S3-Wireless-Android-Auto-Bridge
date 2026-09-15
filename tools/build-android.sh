#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=== Building Android Companion Application ==="
cd "$REPO_ROOT"

if [ -f "./gradlew" ]; then
    ./gradlew assembleDebug
elif [ -d "android-app" ] && [ -f "android-app/gradlew" ]; then
    cd android-app && ./gradlew assembleDebug
else
    gradle assembleDebug
fi

echo "=== Android Build Finished ==="
