#!/usr/bin/env bash
set -euo pipefail

# 1) build the Flutter Linux bundle for arm64
flutter build linux

# 2) define your source & destination
BUILD_DIR="/home/pi/src/build/linux/arm64/release/bundle"
DEPLOY_DIR="/home/pi/console"

# 3) sync new/updated files (no deletion)
rsync -a "$BUILD_DIR"/ "$DEPLOY_DIR"/

echo "✅ Synced updated files from $BUILD_DIR → $DEPLOY_DIR"
echo "Rebooting now..."

sudo reboot
