#pragma once

#include "types.h"
#include <QString>
#include <QList>

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
 *   - [ ] Another task
 *
 * Projects are loaded in alphabetical filename order.
 */
class MarkdownIO
{
public:
    /** Load all *.md files from dir as projects. Creates dir if absent. */
    static QList<Project> loadAll(const QString &dir);

    /** Write project to <dir>/<project.name>.md (overwrites). */
    static void save(const QString &dir, const Project &project);

    /** Delete the .md file for the named project. */
    static void remove(const QString &dir, const QString &projectName);

    /** Rename the .md file; caller must already have updated project.name. */
    static void rename(const QString &dir, const QString &oldName, const QString &newName);

private:
    static Project parseFile(const QString &filePath, const QString &projectName);
    static QString filePath(const QString &dir, const QString &name);
};
