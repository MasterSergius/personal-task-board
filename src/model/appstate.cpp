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

bool AppState::validProject(int idx) const
{
    return idx >= 0 && idx < m_projects.size();
}

bool AppState::validColumn(int projectIdx, int colIdx) const
{
    return validProject(projectIdx)
        && colIdx >= 0 && colIdx < m_projects[projectIdx].columns.size();
}

Task *AppState::taskAt(int projectIdx, int colIdx, int taskIdx)
{
    if (!validColumn(projectIdx, colIdx))
        return nullptr;
    auto &tasks = m_projects[projectIdx].columns[colIdx].tasks;
    if (taskIdx < 0 || taskIdx >= tasks.size())
        return nullptr;
    return &tasks[taskIdx];
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

bool AppState::isValidProjectName(const QString &name)
{
    if (name.isEmpty() || name != name.trimmed() || name.startsWith('.'))
        return false;

    static const QString forbidden = QStringLiteral("/\\:*?\"<>|");
    for (const QChar c : name)
        if (forbidden.contains(c) || c.category() == QChar::Other_Control)
            return false;

    return true;
}

bool AppState::projectNameTaken(const QString &name, int exceptIdx) const
{
    // Case-insensitive: "Work" and "work" map to the same file on
    // Windows/macOS file systems.
    for (int i = 0; i < m_projects.size(); ++i)
        if (i != exceptIdx && m_projects[i].name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

int AppState::addProject(const QString &name)
{
    if (!isValidProjectName(name) || projectNameTaken(name))
        return -1;

    m_projects.append(Project{name, {}});
    int idx = m_projects.size() - 1;
    save(idx);
    return idx;
}

void AppState::removeProject(int idx)
{
    if (!validProject(idx))
        return;
    MarkdownIO::remove(m_boardsDir, m_projects[idx].name);
    m_projects.removeAt(idx);
}

bool AppState::renameProject(int idx, const QString &newName)
{
    if (!validProject(idx) || !isValidProjectName(newName) || projectNameTaken(newName, idx))
        return false;

    if (!MarkdownIO::rename(m_boardsDir, m_projects[idx].name, newName))
        return false;

    m_projects[idx].name = newName;
    save(idx);   // also covers a project whose file did not exist yet
    return true;
}

// ─── Columns ──────────────────────────────────────────────────────────────────

void AppState::addColumn(int projectIdx, const QString &name)
{
    if (!validProject(projectIdx))
        return;
    m_projects[projectIdx].columns.append(Column{name, {}});
    save(projectIdx);
}

void AppState::removeColumn(int projectIdx, int colIdx)
{
    if (!validColumn(projectIdx, colIdx))
        return;
    m_projects[projectIdx].columns.removeAt(colIdx);
    save(projectIdx);
}

void AppState::renameColumn(int projectIdx, int colIdx, const QString &name)
{
    if (!validColumn(projectIdx, colIdx))
        return;
    m_projects[projectIdx].columns[colIdx].name = name;
    save(projectIdx);
}

void AppState::moveColumn(int projectIdx, int from, int to)
{
    if (from == to || !validColumn(projectIdx, from) || !validColumn(projectIdx, to))
        return;
    auto &cols = m_projects[projectIdx].columns;
    cols.insert(to, cols.takeAt(from));
    save(projectIdx);
}

void AppState::moveColumnLeft(int projectIdx, int colIdx)
{
    moveColumn(projectIdx, colIdx, colIdx - 1);
}

void AppState::moveColumnRight(int projectIdx, int colIdx)
{
    moveColumn(projectIdx, colIdx, colIdx + 1);
}

// ─── Tasks ────────────────────────────────────────────────────────────────────

void AppState::addTask(int projectIdx, int colIdx, const QString &title)
{
    if (!validColumn(projectIdx, colIdx))
        return;
    m_projects[projectIdx].columns[colIdx].tasks.append(Task{title});
    save(projectIdx);
}

void AppState::removeTask(int projectIdx, int colIdx, int taskIdx)
{
    if (!validColumn(projectIdx, colIdx))
        return;
    auto &tasks = m_projects[projectIdx].columns[colIdx].tasks;
    if (taskIdx < 0 || taskIdx >= tasks.size())
        return;
    tasks.removeAt(taskIdx);
    save(projectIdx);
}

void AppState::moveTask(int projectIdx, int fromColIdx, int taskIdx, int toColIdx)
{
    if (!validColumn(projectIdx, fromColIdx) || !validColumn(projectIdx, toColIdx))
        return;
    auto &from = m_projects[projectIdx].columns[fromColIdx].tasks;
    if (taskIdx < 0 || taskIdx >= from.size())
        return;
    Task task = from.takeAt(taskIdx);
    m_projects[projectIdx].columns[toColIdx].tasks.append(task);
    save(projectIdx);
}

void AppState::setTaskPriority(int projectIdx, int colIdx, int taskIdx, Priority priority)
{
    Task *task = taskAt(projectIdx, colIdx, taskIdx);
    if (!task || task->priority == priority)
        return;
    task->priority = priority;
    save(projectIdx);
}

bool AppState::setTaskColor(int projectIdx, int colIdx, int taskIdx, const QString &color)
{
    Task *task = taskAt(projectIdx, colIdx, taskIdx);
    if (!task)
        return false;

    const QString normalized = MarkdownIO::normalizeColor(color);
    if (!color.isEmpty() && normalized.isEmpty())
        return false;

    if (task->color != normalized) {
        task->color = normalized;
        save(projectIdx);
    }
    return true;
}
