# Upwork screenshots on Hyprland / Omarchy

Official Upwork does **not** support Wayland. On Hyprland it uploads a grey
image that says *"Upwork Screenshots are not supported on Wayland, please
switch to Xorg."*

This repo keeps you on Hyprland. It does **not** switch to GNOME or X11.

## After an Omarchy reset

Install the Upwork desktop app first (`/usr/bin/upwork`, AUR `upwork` package), then:

```bash
git clone https://github.com/niiithish/upwork-wayland-hyprland.git
cd upwork-wayland-hyprland
./install.sh
```

Fully quit Upwork (tray included) and start it with `upwork` or the app launcher.

### Paste this to Grok

> Clone https://github.com/niiithish/upwork-wayland-hyprland and run `./install.sh`. This restores Upwork time-tracker screenshots on Omarchy/Hyprland without switching to Xorg or GNOME. Read this repo's README first — it has the full diagnosis. Fully quit Upwork, then launch it with the new `upwork` wrapper. Do not install GNOME and do not switch the session to X11.

---

## Agent notes (for Grok / any assistant)

This section is the memory. If the machine was reset and the user asks to fix Upwork screenshots again, read this whole file, then run `./install.sh`. Do not reinvent a GNOME/Xorg session.

### Problem

- User: freelancer on Upwork.
- Distro: [Omarchy](https://omarchy.org/) (Arch + **Hyprland**, Wayland).
- Symptom: time-tracker memo is a solid grey image with
  `Upwork Screenshots are not supported on Wayland, please switch to Xorg.`
- Constraint: **do not** switch to GNOME, KDE, or a full Xorg session.

Omarchy skill applies for desktop config (`~/.config/hypr/`, `~/.config/omarchy/`).
Do **not** edit `/usr/share/omarchy/`. This workaround lives entirely in the
user's home + this git repo.

### Why the grey image happens

Upwork's native module `/usr/lib/upwork/uta_native.node` (linked from the Electron
app) has an `isWayland` check. On Wayland it **does not capture**; it uploads
that placeholder on purpose.

The same module *can* capture via GDK on X11:

- `gdk_pixbuf_get_from_window` (undefined symbol, dynamically linked — so
  `LD_PRELOAD` works)
- also talks to `org.gnome.Shell.Screenshot`, KDE screenshot, and
  `org.freedesktop.portal.Screenshot`
- idle via `org.gnome.ScreenSaver` / Mutter IdleMonitor / XScreenSaver

On Hyprland those GNOME D-Bus APIs are absent. Portal capture would prompt
every memo. So we do **not** rely on portals.

### What this workaround does

1. Wrapper (`upwork-wayland`) lies to Upwork:
   - `XDG_SESSION_TYPE=x11`
   - unset `WAYLAND_DISPLAY` (saved as `WAYLAND_DISPLAY_REAL` for grim)
   - `GDK_BACKEND=x11`
   - `ELECTRON_OZONE_PLATFORM_HINT=x11`
2. Upwork's **window** therefore runs as **XWayland** (needs `DISPLAY`, which
   Omarchy/Hyprland already provides).
3. `LD_PRELOAD` `gdk-screenshotter.so` intercepts
   `gdk_pixbuf_get_from_window()` and runs `grim -t png /tmp/upwork-screenshot.png`.
4. grim talks to Hyprland (`wlr-screencopy`) using `WAYLAND_DISPLAY_REAL`, so
   the image is the real Wayland desktop, including native Wayland apps.
5. Extra intercepts (from upstream):
   - `gdk_pixbuf_save_to_callback` + spoiling `XGetWindowAttributes` width/height
     so Electron does not replace the native snapshot with a black “enhanced”
     one.
   - `XGetWindowProperty` for `_NET_WM_NAME` / `_NET_WM_PID` via
     `hyprctl activewindow -j` (upstream used `swaymsg`; that is wrong here).

Verified on this setup: grim through the hook produced a 1920×1080 JPEG.
A live Upwork memo still has to be checked after login.

### Do not

- Do not tell the user to log into “Ubuntu on Xorg” / GNOME / a second DE.
- Do not use `upwork-wlroots-bridge` as the primary fix for *this* Upwork
  build: the grey image is an explicit Wayland refusal, not a missing GNOME
  Screenshot daemon. The GDK intercept is the path that matches
  `uta_native.node`.
- Do not use flameshot (upstream default). Omarchy already has **grim**.
- Do not edit `/usr/share/omarchy/` or Upwork package binaries.

### Files this install creates

| Path | Role |
|------|------|
| `~/.local/share/upwork-wayland/gdk-screenshotter.so` | LD_PRELOAD hook |
| `~/.local/share/upwork-wayland/{gdk-screenshotter.c,Makefile}` | rebuild sources |
| `~/.local/bin/upwork-wayland` | launcher |
| `~/.local/bin/upwork` | symlink → launcher (`~/.local/bin` is first on Omarchy PATH) |
| `~/.local/share/applications/upwork.desktop` | overrides `/usr/share/applications/upwork.desktop` |

Wrapper always execs `/usr/bin/upwork` (or `/opt/Upwork/upwork` if that older layout is still present). Never execs itself.

### Restore procedure for an agent

1. Confirm session: `echo $XDG_SESSION_TYPE $XDG_CURRENT_DESKTOP` → `wayland Hyprland`.
2. Confirm Upwork: `test -x /usr/bin/upwork`. If missing, user must install
   the AUR `upwork` package first.
3. Clone this repo if needed, `cd` into it, run `./install.sh`.
4. If deps fail, on Omarchy/Arch:
   `sudo pacman -S gcc make pkgconf gdk-pixbuf2 libx11 libxss grim jq`
   (use a real terminal for sudo; do not hang an agent on a password prompt).
5. `pkill -f '/usr/lib/upwork/app.asar'` if it is already running.
6. User starts tracker with `upwork`.
7. Verify after a memo: `cat /tmp/upwork-wayland.log` should contain
   `screenshot pixbuf WxH`. If grim failed, `WAYLAND_DISPLAY_REAL` was empty
   or grim is broken (`grim /tmp/t.png` should work in a Hyprland session).

### Rebuild

```bash
make -C ~/.local/share/upwork-wayland
# or from this repo:
make && ./install.sh
```

### Uninstall

```bash
./uninstall.sh
```

Removes only the wrapper. Stock `/usr/bin/upwork` stays.

### Caveats

- Unofficial. An Upwork update can change capture and break the hook.
- Idle detection on Hyprland may still be imperfect.
- Hyprland XWayland must stay enabled (Omarchy default).

## Dependencies (Omarchy / Arch)

Usually already installed:

```bash
sudo pacman -S gcc make pkgconf gdk-pixbuf2 libx11 libxss grim jq
```

Plus Hyprland (`hyprctl`) and Upwork at `/usr/bin/upwork`.

## Credits

- [tiesselune/upwork-wayland](https://github.com/tiesselune/upwork-wayland) (GDK `LD_PRELOAD`, GPL-3.0)
- [MarSoft/upwork-wayland](https://github.com/MarSoft/upwork-wayland) (original idea)
- This fork: grim + `hyprctl` for Hyprland/Omarchy instead of flameshot + `swaymsg`.
