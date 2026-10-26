#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/out/build/linux-release"

echo "[1/4] Installing build/runtime dependencies..."
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build \
  fcitx5 fcitx5-config-qt fcitx5-frontend-all \
  libfcitx5core-dev

echo "[2/4] Building BulgarianIME + Fcitx5 addon..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DBGIME_BUILD_FCITX5=ON
cmake --build "${BUILD_DIR}"

echo "[3/4] Installing addon into Fcitx5..."
sudo cmake --install "${BUILD_DIR}"

echo "[4/4] Restarting Fcitx5 if it is already running..."
if command -v fcitx5 >/dev/null 2>&1; then
  fcitx5 -rd >/dev/null 2>&1 || true
fi

cat <<'MSG'

BulgarianIME Linux addon is installed.

FIRST-TIME UBUNTU SETUP:
  1) Run: im-config
  2) Select fcitx5 as the input method framework.
  3) Log out of Ubuntu and log back in.
  4) Run: fcitx5-configtool
  5) Click "Add Input Method..."
  6) Search for "BulgarianIME" and add it.
  7) Switch to BulgarianIME from the Fcitx5 tray/menu or your Fcitx hotkey.

Quick test:
  Type: zdravey az sım şt
  Expected: здравей аз съм щ
MSG
