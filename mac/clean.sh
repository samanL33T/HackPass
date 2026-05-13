#!/usr/bin/env bash
# Wipe local HackPass state for a fresh first-run experience.

set -euo pipefail

echo "Stopping HackPass processes..."
pkill -f HackPass.app/Contents/MacOS/HackPass 2>/dev/null || true
pkill -f hackpass-server 2>/dev/null || true

echo "Wiping ~/Library/Application Support/HackPass/..."
rm -rf "$HOME/Library/Application Support/HackPass"

echo "Wiping ~/Library/Caches/HackPass/..."
rm -rf "$HOME/Library/Caches/HackPass"

echo "Wiping QSettings plist..."
defaults delete com.samanl33t.hackpass 2>/dev/null || true
rm -f "$HOME/Library/Preferences/com.samanl33t.hackpass.plist"

echo
echo "Clean. Next launch will be a first run."
echo "Remove the loaded extension manually if you want a clean test of the first-run flow."
