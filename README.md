# Personal Task Board

A minimalistic multi-project Kanban board for personal use, built with C++ and Qt 6.

*WARNING*: This project contains 99% of "vibe-code"

## UI
<img width="1208" height="737" alt="image" src="https://github.com/user-attachments/assets/27ea9ad6-7a6b-4331-a8f6-2402135e93e5" />

## Features

- **Multiple projects** — create, rename, and delete projects from the left panel
- **Columns** — add, rename, reorder (left/right), and delete columns via right-click on the column header
- **Tasks** — add tasks via the "+ Add Task" button; delete via right-click on a card
- **Task details** — double-click a card (or right-click → *Open…*) to edit its title and a rich-text description: bold, italic, strikethrough, code, headings, bullet / numbered / check lists, links and images. Paste screenshots or drop image files straight into the editor. Cards show a short preview of the description and check-list progress (`☑ 2/5`)
- **Priority & frame color** — right-click a card to set a priority (Urgent / High / Medium / Low, shown as a colored badge) and a frame color (preset palette or any custom color)
- **Drag & drop** — move tasks between columns by dragging
- **Persistent storage** — every change is saved immediately (atomically) to a Markdown file in `~/.ptb/projects/`

## Markdown file format

Each project is stored as `~/.ptb/projects/<project-name>.md`:

```markdown
## To Do

- [ ] Player movement [priority:: high]
    Use **acceleration**, not fixed speed.

    - [x] Walking
    - [ ] Jumping

    ![image](attachments/20261030-153012-a1b2c3.png)
- [ ] Enemy AI [color:: #3e63dd]

## In Progress

- [ ] Inventory system

## Done

- [ ] Project setup
```

The format is compatible with the [Obsidian Kanban plugin](https://github.com/mgmeyers/obsidian-kanban).
Priority and frame color are stored as [Dataview](https://blacksmithgu.github.io/obsidian-dataview/)
inline fields — `[priority:: low|medium|high|urgent]` and `[color:: #rrggbb]` — so they stay readable
in Obsidian. Unrecognised values are left untouched in the task title.
A task's description is Markdown, indented under the task line (lines indented with a tab or
two spaces are read as well). Images pasted into a description are saved as files in
`~/.ptb/projects/attachments/` when the task is saved, and linked by a relative path. Deleting a
task does not delete its images.
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
