#pragma once

#include "types.h"
#include <QString>
#include <QList>
#include <optional>

/**
 * Reads and writes Kanban boards as Markdown files.
 *
 * One file per project, stored in a configurable directory.
 * The project name is the filename stem (without ".md").
 *
 * File format (Obsidian Kanban-compatible):
 *
 *   ## Column Name
 *
 *   - [ ] Pending task
 *   - [x] Completed task
 *
 *   ## Another Column
 *
 *   - [ ] Another task [priority:: high] [color:: #e5484d]
 *
 * Task priority and frame color are stored as Dataview inline fields.
 * Anything before the first column heading (e.g. YAML front matter) and
 * everything from the first "***" or "%%" line onward (Obsidian's archive and
 * settings blocks) is preserved verbatim on save.
 *
 * Projects are loaded in alphabetical filename order.
 */
class MarkdownIO
{
public:
    /** Load all *.md files from dir as projects. Returns empty if dir is absent. */
    static QList<Project> loadAll(const QString &dir);

    /** Parse a single board from Markdown text. */
    static Project parse(const QString &text, const QString &projectName);

    /** Serialise a board to Markdown text (inverse of parse()). */
    static QString serialize(const Project &project);

    /**
     * Write project to <dir>/<project.name>.md atomically (overwrites).
     * Creates dir if absent. Returns false on I/O failure.
     */
    static bool save(const QString &dir, const Project &project);

    /** Delete the .md file for the named project. */
    static bool remove(const QString &dir, const QString &projectName);

    /**
     * Rename the .md file. Succeeds trivially if there is no old file.
     * Returns false if the rename fails (e.g. the target already exists).
     */
    static bool rename(const QString &dir, const QString &oldName, const QString &newName);

    // ── Task metadata helpers ────────────────────────────────────────────────

    /** "low" / "medium" / "high" / "urgent"; empty for Priority::None. */
    static QString priorityName(Priority priority);

    /** Inverse of priorityName() (case-insensitive); nullopt if unrecognised. */
    static std::optional<Priority> priorityFromName(const QString &name);

    /** "#RGB" / "#RRGGBB" → lowercase "#rrggbb"; empty if not a valid hex color. */
    static QString normalizeColor(const QString &color);

private:
    static QString filePath(const QString &dir, const QString &name);
};
