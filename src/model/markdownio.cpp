#include "markdownio.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

// ─── Private helpers ──────────────────────────────────────────────────────────

QString MarkdownIO::filePath(const QString &dir, const QString &name)
{
    return dir + "/" + name + ".md";
}

Project MarkdownIO::parseFile(const QString &path, const QString &projectName)
{
    Project project;
    project.name = projectName;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return project;

    QTextStream in(&file);
    Column *currentColumn = nullptr;

    while (!in.atEnd()) {
        const QString line = in.readLine();

        if (line.startsWith("## ")) {
            // Start a new column
            project.columns.append(Column{line.mid(3).trimmed(), {}});
            currentColumn = &project.columns.last();
        } else if ((line.startsWith("- [ ] ") || line.startsWith("- [x] ")) && currentColumn) {
            // Task entry — treat both checked and unchecked as a task
            currentColumn->tasks.append(Task{line.mid(6).trimmed()});
        }
    }

    return project;
}

// ─── Public API ───────────────────────────────────────────────────────────────

QList<Project> MarkdownIO::loadAll(const QString &dir)
{
    QList<Project> projects;

    QDir d(dir);
    if (!d.exists())
        return projects;

    const QStringList files = d.entryList({"*.md"}, QDir::Files, QDir::Name);
    for (const QString &filename : files) {
        const QString name = QFileInfo(filename).baseName();
        projects.append(parseFile(dir + "/" + filename, name));
    }

    return projects;
}

void MarkdownIO::save(const QString &dir, const Project &project)
{
    QDir().mkpath(dir);

    QFile file(filePath(dir, project.name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return;

    QTextStream out(&file);

    for (int i = 0; i < project.columns.size(); ++i) {
        const Column &col = project.columns[i];

        out << "## " << col.name << "\n\n";

        for (const Task &task : col.tasks)
            out << "- [ ] " << task.title << "\n";

        // Blank line between columns
        if (i < project.columns.size() - 1)
            out << "\n";
    }
}

void MarkdownIO::remove(const QString &dir, const QString &projectName)
{
    QFile::remove(filePath(dir, projectName));
}

void MarkdownIO::rename(const QString &dir, const QString &oldName, const QString &newName)
{
    QFile::rename(filePath(dir, oldName), filePath(dir, newName));
}
