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
 * Index parameters are always zero-based and assumed to be in range;
 * callers (UI layer) are responsible for bounds-checking before calling.
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
     * Add a new project with default empty column list.
     * Returns the index of the new project, or -1 if the name already exists.
     */
    int addProject(const QString &name);

    /** Remove project at idx and delete its .md file. */
    void removeProject(int idx);

    /**
     * Rename project at idx.
     * Returns false if newName is already taken by another project.
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

    QString m_boardsDir;
    QList<Project> m_projects;
};
