#pragma once

#include "types.h"
#include <QString>
#include <QList>
#include <QStringList>

/**
 * Central application state.
 *
 * Owns all project/column/task data and coordinates persistence.
 * Every mutating method saves the affected project to disk automatically.
 *
 * Index parameters are zero-based. Mutators silently ignore out-of-range
 * indices so a stale index from the UI can never corrupt memory.
 */
class AppState
{
public:
    explicit AppState(const QString &boardsDir);

    /** Load all projects from disk. Call once at startup. */
    void load();

    // ── Projects ─────────────────────────────────────────────────────────────

    int projectCount() const;
    const Project &project(int idx) const;
    QStringList projectNames() const;

    /**
     * Whether name can be used as a project (and therefore file) name:
     * non-empty, no surrounding whitespace, no leading '.', and none of the
     * characters that are illegal in filenames on Windows/macOS/Linux.
     */
    static bool isValidProjectName(const QString &name);

    /** Whether another project (other than exceptIdx) already uses name. Case-insensitive. */
    bool projectNameTaken(const QString &name, int exceptIdx = -1) const;

    /**
     * Add a new project with default empty column list.
     * Returns the index of the new project, or -1 if the name is invalid
     * or already exists.
     */
    int addProject(const QString &name);

    /** Remove project at idx and delete its .md file. */
    void removeProject(int idx);

    /**
     * Rename project at idx.
     * Returns false if newName is invalid, already taken by another project,
     * or the file could not be renamed on disk.
     */
    bool renameProject(int idx, const QString &newName);

    // ── Columns ──────────────────────────────────────────────────────────────

    void addColumn(int projectIdx, const QString &name);
    void removeColumn(int projectIdx, int colIdx);
    void renameColumn(int projectIdx, int colIdx, const QString &name);

    /** Move column from position 'from' to position 'to' (order preserved). */
    void moveColumn(int projectIdx, int from, int to);
    void moveColumnLeft(int projectIdx, int colIdx);
    void moveColumnRight(int projectIdx, int colIdx);

    // ── Tasks ────────────────────────────────────────────────────────────────

    void addTask(int projectIdx, int colIdx, const QString &title);
    void removeTask(int projectIdx, int colIdx, int taskIdx);

    /** Move task from one column to another (appended at the end). */
    void moveTask(int projectIdx, int fromColIdx, int taskIdx, int toColIdx);

private:
    void save(int projectIdx) const;

    bool validProject(int idx) const;
    bool validColumn(int projectIdx, int colIdx) const;

    QString m_boardsDir;
    QList<Project> m_projects;
};
