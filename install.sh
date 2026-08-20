#!/usr/bin/env bash
# Install the Upwork-on-Hyprland screenshot wrapper for the current user.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PREFIX="${UPWORK_WAYLAND_DIR:-$HOME/.local/share/upwork-wayland}"
BIN="${XDG_BIN_HOME:-$HOME/.local/bin}"
APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing command: $1" >&2
    echo "On Omarchy/Arch install it, e.g.: sudo pacman -S $2" >&2
    exit 1
  }
}

need_pkg() {
  pkg-config --exists "$1" || {
    echo "Missing pkg-config package: $1" >&2
    echo "On Omarchy/Arch: sudo pacman -S $2" >&2
    exit 1
  }
}

need_cmd gcc gcc
need_cmd make make
need_cmd pkg-config pkgconf
need_cmd grim grim
need_cmd jq jq
need_cmd hyprctl hyprland
need_pkg gdk-pixbuf-2.0 gdk-pixbuf2
need_pkg x11 libx11
need_pkg xscrnsaver libxss

if [[ ! -x /opt/Upwork/upwork ]]; then
  echo "Warning: /opt/Upwork/upwork not found yet." >&2
  echo "Install the Upwork desktop app, then launch it with: upwork" >&2
fi

echo "Building screenshot hook..."
make -C "$ROOT"

mkdir -p "$PREFIX" "$BIN" "$APPS"
install -m 644 "$ROOT/gdk-screenshotter.c" "$PREFIX/"
install -m 644 "$ROOT/Makefile" "$PREFIX/"
install -m 755 "$ROOT/gdk-screenshotter.so" "$PREFIX/"
install -m 755 "$ROOT/upwork-wayland" "$BIN/upwork-wayland"
ln -sfn "$BIN/upwork-wayland" "$BIN/upwork"

cat > "$APPS/upwork.desktop" <<EOF
[Desktop Entry]
Name=Upwork
Exec=$BIN/upwork-wayland %U
Terminal=false
Type=Application
Icon=upwork
StartupWMClass=Upwork
StartupNotify=false
MimeType=x-scheme-handler/upwork;
Comment=Upwork Desktop Application (Wayland screenshot wrapper)
Categories=Utility;Network;
EOF

update-desktop-database "$APPS" 2>/dev/null || true

case ":$PATH:" in
  *":$BIN:"*) ;;
  *)
    echo "Note: $BIN is not on PATH. Add it so the 'upwork' command works." >&2
    ;;
esac

echo
echo "Installed."
echo "  wrapper: $BIN/upwork-wayland"
echo "  hook:    $PREFIX/gdk-screenshotter.so"
echo
echo "Fully quit Upwork if it is running, then start it with: upwork"
echo "After a memo screenshot, check: /tmp/upwork-wayland.log"
