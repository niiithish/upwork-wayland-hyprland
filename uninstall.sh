#!/usr/bin/env bash
set -euo pipefail

PREFIX="${UPWORK_WAYLAND_DIR:-$HOME/.local/share/upwork-wayland}"
BIN="${XDG_BIN_HOME:-$HOME/.local/bin}"
APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"

rm -f "$BIN/upwork-wayland"
if [[ -L "$BIN/upwork" ]]; then
  target="$(readlink -f "$BIN/upwork" || true)"
  if [[ "$target" == "$BIN/upwork-wayland" ]]; then
    rm -f "$BIN/upwork"
  fi
fi
rm -f "$APPS/upwork.desktop"
rm -rf "$PREFIX"
update-desktop-database "$APPS" 2>/dev/null || true
echo "Removed the Upwork Wayland screenshot wrapper."
echo "The stock Upwork app (/usr/bin/upwork) is unchanged."
