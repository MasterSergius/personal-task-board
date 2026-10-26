# Personal Task Board

A minimalistic multi-project Kanban board for personal use, built with C++ and Qt 6.

*WARNING*: This project contains 99% of "vibe-code"

## UI
<img width="1208" height="737" alt="image" src="https://github.com/user-attachments/assets/27ea9ad6-7a6b-4331-a8f6-2402135e93e5" />

## Features

- **Multiple projects** — create, rename, and delete projects from the left panel
- **Columns** — add, rename, reorder (left/right), and delete columns via right-click on the column header
- **Tasks** — add tasks via the "+ Add Task" button; delete via right-click on a card
- **Drag & drop** — move tasks between columns by dragging
- **Persistent storage** — every change is saved immediately (atomically) to a Markdown file in `~/.ptb/projects/`

## Markdown file format

Each project is stored as `~/.ptb/projects/<project-name>.md`:

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
Checked tasks (`- [x]`) keep their state, and content the app doesn't edit — YAML front matter,
the `***` archive section and the `%% kanban:settings` block — is preserved verbatim on save.

Project names become file names, so they may not start with `.` or contain `/ \ : * ? " < > |`,
and names differing only in letter case are treated as duplicates.

## Building

Requires Qt 6 and CMake 3.16+.

```bash
make          # configure + build (outputs build/ptb)
make run      # build and run
make test     # build and run the test suite
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
