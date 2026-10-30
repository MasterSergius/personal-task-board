# Changelog

## [Unreleased]

### Fixed
- Possible use-after-free when dropping a card or column: the board was rebuilt (deleting the dragged widget and its `QDrag`) while `QDrag::exec()` was still running. The drag source now requests the move after `exec()` returns
- Plain text dragged in from other applications (e.g. `"3:7"`) was treated as a task move and could index out of range; drags now use private MIME types and are accepted only from this application
- Completed tasks (`- [x]`) were rewritten as `- [ ]` on the next save; `- [X]` was not recognised at all
- Saving an Obsidian Kanban board dropped its front matter, archive section and settings block
- Project names containing dots were truncated on reload (`v1.2 plan` → `v1`)
- Project names containing `/`, `..` or a leading `.` could write outside the boards directory or create hidden files that were never loaded again; such names are now rejected
- Projects whose names differ only in case could overwrite each other's file on case-insensitive file systems
- Renaming a project ignored file-system errors, leaving the old file behind
- UTF-8 BOM at the start of a board file hid the first column
- Indented lines under a task (e.g. notes written in Obsidian) were dropped on the next save; they are now kept as the task's description

### Changed
- Board files are written atomically via `QSaveFile`
- `AppState` mutators ignore out-of-range indices instead of invoking undefined behaviour
- `MarkdownIO::save/remove/rename` return `bool`; added `MarkdownIO::parse/serialize`

### Added
- Regression tests for the fixes above, plus a headless `ColumnWidget` drag-and-drop test
- Task priority (Urgent / High / Medium / Low), shown as a colored badge on the card
- Custom task frame color: preset palette or any color via a color picker
- Both are set from the card's right-click menu and stored as Dataview inline fields (`[priority:: high]`, `[color:: #e5484d]`)
- Tests for parsing, serialisation, persistence and card rendering of the new fields
- Task editor dialog, opened by double-clicking a card or via *Open…* in its context menu: edit the title and a rich-text description (bold, italic, strikethrough, inline code, headings, bullet / numbered / check lists, links, images)
- Images can be pasted or dropped into the description; they are written to `attachments/` next to the boards only when the task is saved
- Descriptions are stored as Markdown indented under the task line; cards show a description preview and check-list progress
- Tests for description parsing / serialisation, the editor's Markdown round trip, image attachments, the toolbar and card preview

## [0.4.0] — 2026-04-15

### Added
- Unit tests for `MarkdownIO` and `AppState` using Qt Test (`tests/`)
- GitHub Actions CI workflow: builds the project and runs tests on every push and pull request

### Changed
- Model code extracted into a `ptb_model` static library so tests can link it without pulling in Qt::Widgets

## [0.3.0] — 2026-04-15

### Changed
- Removed hardcoded sample project — app starts with an empty board on first run

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
