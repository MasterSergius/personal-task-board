# Personal Task Board

A minimalistic multi-project Kanban board for personal use, built with C++ and Qt 6.

*WARNING*: This project contains 99% of "vibe-code"

## Features

- **Multiple projects** — create, rename, and delete projects from the left panel
- **Columns** — add, rename, reorder (left/right), and delete columns via right-click on the column header
- **Tasks** — add tasks via the "+ Add Task" button; delete via right-click on a card
- **Drag & drop** — move tasks between columns by dragging
- **Persistent storage** — every change is saved immediately to a Markdown file in the `boards/` directory next to the executable

## Markdown file format

Each project is stored as `boards/<project-name>.md`:

```markdown
## To Do

- [ ] Player movement
- [ ] Enemy AI

## In Progress

- [ ] Inventory system

## Done

- [ ] Project setup
```

The format is compatible with the [Obsidian Kanban plugin](https://github.com/mgmeyers/obsidian-kanban).

## Building

Requires Qt 6 and CMake 3.16+.

```bash
make          # configure + build (outputs build/ptb)
make run      # build and run
make clean    # remove build/
```

Or manually:

```bash
cmake -S . -B build
cmake --build build
./build/ptb
```

## Linux troubleshooting

If the app fails with `Could not load the Qt platform plugin "xcb"`, install the missing system libraries:

| Distribution | Command |
|---|---|
| Void Linux | `sudo xbps-install -S xcb-util-cursor xcb-util-wm xcb-util-image xcb-util-keysyms libxkbcommon-x11` |
| Ubuntu/Debian | `sudo apt install libxcb-cursor0 libxcb-xinerama0 libxkbcommon-x11-0` |
| Arch Linux | `sudo pacman -S xcb-util-cursor` |

Quick workaround (Wayland):

```bash
QT_QPA_PLATFORM=wayland ./build/ptb
```
