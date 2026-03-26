# Personal Task Board

It's a minimalistic kanban board for personal use (i.e. single-user mode only) based on QT

## WARNING: This product is 100% "vibecoded"

## Linux (Qt / PySide6) Troubleshooting

On some Linux distributions, the application may fail to start with an error like:

```
qt.qpa.plugin: Could not load the Qt platform plugin "xcb"
```

This usually means required system libraries for Qt are missing.

### Void Linux

Install required dependencies:

```
sudo xbps-install -S xcb-util-cursor xcb-util-wm xcb-util-image xcb-util-keysyms libxkbcommon-x11
```

### Ubuntu / Debian

```
sudo apt install libxcb-cursor0 libxcb-xinerama0 libxkbcommon-x11-0
```

### Arch Linux

```
sudo pacman -S xcb-util-cursor
```

---

### Quick workaround

You can try running the app with Wayland:

```
QT_QPA_PLATFORM=wayland uv run app/main.py
```

---

Qt relies on system-level libraries for platform integration, which are not installed automatically via Python dependencies.
