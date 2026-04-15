# Changelog

## [0.2.0] — 2026-04-15

### Added
- **Project management**: add, rename, and delete projects from the left sidebar
- **Column management**: add columns via the "+ Add Column" button on the board; rename, move left/right, and delete via right-click on the column header
- **Task management**: add tasks via the "+ Add Task" button; delete via right-click context menu on a card
- **Markdown persistence**: every state change is saved immediately; one `.md` file per project in `boards/` next to the executable
- **Model layer** (`src/model/`): `AppState` for CRUD + auto-save, `MarkdownIO` for file I/O, plain `types.h` structs
- Dark sidebar styling for the project list
- Duplicate-name guards on add/rename for both projects and columns

### Changed
- Drag & drop now encodes `colIdx:taskIdx` in MIME data instead of a raw pointer address, enabling model updates on drop
- `BoardView::refresh()` rebuilds columns from model state after every mutation (deferred via `QTimer::singleShot` to avoid use-after-free in signal handlers)
- All UI → model communication goes through signals/slots; no direct parent-pointer casts between widgets

### Removed
- Hardcoded test data (columns and cards are now loaded from disk)
- Python/PySide6 prototype code

## [0.1.0] — 2026-04-09

- Basic Kanban board with hardcoded "To Do / In Progress / Done" columns
- Drag & drop task movement between columns via raw pointer in MIME data
- Add task dialog (no persistence)
- Project list sidebar (no project switching)
