#!/usr/bin/env bash
set -euo pipefail

# Paths match Fcitx5's default /usr prefix used by install-ubuntu.sh.
sudo rm -f /usr/lib/*/fcitx5/bulgarianime.so 2>/dev/null || true
sudo rm -f /usr/lib/fcitx5/bulgarianime.so 2>/dev/null || true
sudo rm -f /usr/share/fcitx5/addon/bulgarianime.conf
sudo rm -f /usr/share/fcitx5/inputmethod/bulgarianime.conf
fcitx5 -rd >/dev/null 2>&1 || true

echo "BulgarianIME Fcitx5 addon removed."
