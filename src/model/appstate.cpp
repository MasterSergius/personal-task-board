#include "appstate.h"
#include "markdownio.h"

AppState::AppState(const QString &boardsDir)
    : m_boardsDir(boardsDir)
{
}

void AppState::load()
{
    m_projects = MarkdownIO::loadAll(m_boardsDir);
}

void AppState::save(int projectIdx) const
{
    MarkdownIO::save(m_boardsDir, m_projects[projectIdx]);
}

// ─── Projects ─────────────────────────────────────────────────────────────────

int AppState::projectCount() const
{
    return m_projects.size();
}

const Project &AppState::project(int idx) const
{
    return m_projects[idx];
}

QStringList AppState::projectNames() const
{
    QStringList names;
    names.reserve(m_projects.size());
    for (const Project &p : m_projects)
        names.append(p.name);
    return names;
}

int AppState::addProject(const QString &name)
{
    for (const Project &p : m_projects)
        if (p.name == name)
            return -1;

    m_projects.append(Project{name, {}});
    int idx = m_projects.size() - 1;
    save(idx);
    return idx;
}

void AppState::removeProject(int idx)
{
    MarkdownIO::remove(m_boardsDir, m_projects[idx].name);
    m_projects.removeAt(idx);
}

bool AppState::renameProject(int idx, const QString &newName)
{
    for (int i = 0; i < m_projects.size(); ++i)
        if (i != idx && m_projects[i].name == newName)
            return false;

    MarkdownIO::rename(m_boardsDir, m_projects[idx].name, newName);
    m_projects[idx].name = newName;
    return true;
}

// ─── Columns ──────────────────────────────────────────────────────────────────

void AppState::addColumn(int projectIdx, const QString &name)
{
    m_projects[projectIdx].columns.append(Column{name, {}});
    save(projectIdx);
}

void AppState::removeColumn(int projectIdx, int colIdx)
{
    m_projects[projectIdx].columns.removeAt(colIdx);
    save(projectIdx);
}

void AppState::renameColumn(int projectIdx, int colIdx, const QString &name)
{
    m_projects[projectIdx].columns[colIdx].name = name;
    save(projectIdx);
}

void AppState::moveColumnLeft(int projectIdx, int colIdx)
{
    if (colIdx <= 0)
        return;
    m_projects[projectIdx].columns.swapItemsAt(colIdx - 1, colIdx);
    save(projectIdx);
}

void AppState::moveColumnRight(int projectIdx, int colIdx)
{
    auto &cols = m_projects[projectIdx].columns;
    if (colIdx >= cols.size() - 1)
        return;
    cols.swapItemsAt(colIdx, colIdx + 1);
    save(projectIdx);
}

// ─── Tasks ────────────────────────────────────────────────────────────────────

void AppState::addTask(int projectIdx, int colIdx, const QString &title)
{
    m_projects[projectIdx].columns[colIdx].tasks.append(Task{title});
    save(projectIdx);
}

void AppState::removeTask(int projectIdx, int colIdx, int taskIdx)
{
    m_projects[projectIdx].columns[colIdx].tasks.removeAt(taskIdx);
    save(projectIdx);
}

void AppState::moveTask(int projectIdx, int fromColIdx, int taskIdx, int toColIdx)
{
    Task task = m_projects[projectIdx].columns[fromColIdx].tasks.takeAt(taskIdx);
    m_projects[projectIdx].columns[toColIdx].tasks.append(task);
    save(projectIdx);
}
