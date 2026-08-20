# Upwork screenshots on Hyprland / Omarchy

Official Upwork does **not** support Wayland. On Hyprland it uploads a grey
image that says *"Upwork Screenshots are not supported on Wayland, please
switch to Xorg."*

This repo keeps you on Hyprland. It does **not** switch to GNOME or X11.

How it works:

1. Launch Upwork as an XWayland app (so the tracker takes its X11/GDK path).
2. `LD_PRELOAD` a small library that intercepts `gdk_pixbuf_get_from_window()`.
3. Fill that capture with a real **grim** screenshot of the Hyprland desktop.

Adapted from [tiesselune/upwork-wayland](https://github.com/tiesselune/upwork-wayland)
(flameshot / GNOME) for Hyprland: grim + `hyprctl`.

## After an Omarchy reset

Install the Upwork desktop app first, then:

```bash
git clone https://github.com/niiithish/upwork-wayland-hyprland.git
cd upwork-wayland-hyprland
./install.sh
```

Fully quit Upwork (tray included) and start it with `upwork` or the app launcher.

Paste this to Grok if you want it done for you:

> Clone https://github.com/niiithish/upwork-wayland-hyprland and run `./install.sh`. This restores Upwork time-tracker screenshots on Omarchy/Hyprland without switching to Xorg or GNOME. Fully quit Upwork first, then launch it with the new `upwork` wrapper.

## Verify

After a memo screenshot:

```bash
cat /tmp/upwork-wayland.log
```

You want a line like `screenshot pixbuf 1920x1080`, not a grim failure.

## Dependencies (Omarchy / Arch)

Already present on a stock Omarchy install in most cases:

```bash
sudo pacman -S gcc make pkgconf gdk-pixbuf2 libx11 libxss grim jq
```

Plus Hyprland (`hyprctl`) and the Upwork app at `/opt/Upwork/upwork`.

## Uninstall

```bash
./uninstall.sh
```

That only removes the wrapper. Stock Upwork stays.

## Caveats

- This is an unofficial workaround. An Upwork update can break it.
- Idle detection on Hyprland may still be imperfect. Screenshots were the blocker.
- Hyprland must have XWayland enabled (Omarchy does). The Upwork *window* is X11; the *screenshot* is still your Wayland desktop via grim.
