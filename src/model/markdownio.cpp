#include "markdownio.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

// ─── Private helpers ──────────────────────────────────────────────────────────

QString MarkdownIO::filePath(const QString &dir, const QString &name)
{
    return dir + "/" + name + ".md";
}

// Extracts recognised Dataview inline fields ("[priority:: high]",
// "[color:: #e5484d]") from a task title. The first valid field of each kind
// is applied and removed; anything unrecognised stays in the title verbatim.
static void extractFields(Task &task)
{
    static const QRegularExpression fieldRe(
        R"(\s*\[\s*(priority|color)\s*::\s*([^\]]*?)\s*\])",
        QRegularExpression::CaseInsensitiveOption);

    bool havePriority = false;
    bool haveColor    = false;
    QString title = task.title;

    qsizetype pos = 0;
    QRegularExpressionMatch m;
    while ((m = fieldRe.match(title, pos)).hasMatch()) {
        const QString key   = m.captured(1).toLower();
        const QString value = m.captured(2);
        bool consumed = false;

        if (key == "priority" && !havePriority) {
            if (auto p = MarkdownIO::priorityFromName(value)) {
                task.priority = *p;
                havePriority = consumed = true;
            }
        } else if (key == "color" && !haveColor) {
            const QString c = MarkdownIO::normalizeColor(value);
            if (!c.isEmpty()) {
                task.color = c;
                haveColor = consumed = true;
            }
        }

        if (consumed) {
            title.remove(m.capturedStart(), m.capturedLength());
            pos = m.capturedStart();
        } else {
            pos = m.capturedEnd();
        }
    }

    task.title = title.trimmed();
}

// Matches "- [ ] ", "- [x] " and "- [X] ". Sets done accordingly.
static bool parseTaskLine(const QString &line, Task &task)
{
    if (line.size() < 6 || !line.startsWith("- [") || line.mid(4, 2) != "] ")
        return false;

    const QChar mark = line.at(3);
    if (mark != ' ' && mark != 'x' && mark != 'X')
        return false;

    task.title = line.mid(6).trimmed();
    task.done  = (mark != ' ');
    extractFields(task);
    return true;
}

// Removes trailing blank lines / whitespace while keeping leading content intact.
static QString trimTrailing(const QString &s)
{
    qsizetype end = s.size();
    while (end > 0 && s.at(end - 1).isSpace())
        --end;
    return s.left(end);
}

// Width of a line's leading whitespace, counting a tab as 4 columns.
static int indentWidth(const QString &line)
{
    int width = 0;
    for (const QChar c : line) {
        if (c == ' ')
            width += 1;
        else if (c == '\t')
            width += 4;
        else
            break;
    }
    return width;
}

// Removes up to `columns` columns of leading whitespace.
static QString stripIndent(const QString &line, int columns)
{
    int width = 0;
    qsizetype i = 0;
    while (i < line.size() && width < columns) {
        if (line.at(i) == ' ')
            width += 1;
        else if (line.at(i) == '\t')
            width += 4;
        else
            break;
        ++i;
    }
    return line.mid(i);
}

// Turns the indented lines below a task back into its Markdown description.
// The common indentation is removed, but never more than one level (4
// columns), so content that is itself indented (e.g. a code block) keeps it.
static QString dedentDescription(const QStringList &lines)
{
    int common = 4;
    for (const QString &line : lines)
        if (!line.trimmed().isEmpty())
            common = qMin(common, indentWidth(line));

    QStringList out;
    out.reserve(lines.size());
    for (const QString &line : lines)
        out.append(line.trimmed().isEmpty() ? QString() : trimTrailing(stripIndent(line, common)));
    return trimTrailing(out.join('\n'));
}

// ─── Task metadata ────────────────────────────────────────────────────────────

QString MarkdownIO::priorityName(Priority priority)
{
    switch (priority) {
    case Priority::Low:    return "low";
    case Priority::Medium: return "medium";
    case Priority::High:   return "high";
    case Priority::Urgent: return "urgent";
    case Priority::None:   break;
    }
    return {};
}

std::optional<Priority> MarkdownIO::priorityFromName(const QString &name)
{
    const QString n = name.trimmed().toLower();
    for (Priority p : {Priority::Low, Priority::Medium, Priority::High, Priority::Urgent})
        if (n == priorityName(p))
            return p;
    return std::nullopt;
}

QString MarkdownIO::normalizeColor(const QString &color)
{
    static const QRegularExpression hexRe("^#([0-9a-f]{3}|[0-9a-f]{6})$",
                                          QRegularExpression::CaseInsensitiveOption);
    const QString c = color.trimmed().toLower();
    if (!hexRe.match(c).hasMatch())
        return {};
    if (c.size() == 4)   // #rgb → #rrggbb
        return QString("#%1%1%2%2%3%3").arg(c[1]).arg(c[2]).arg(c[3]);
    return c;
}

QString MarkdownIO::withImageAltText(const QString &markdown)
{
    static const QRegularExpression emptyAltRe(R"(!\[\s*\]\()");
    QString out = markdown;
    return out.replace(emptyAltRe, "![image](");
}

// ─── Parsing / serialisation ──────────────────────────────────────────────────

Project MarkdownIO::parse(const QString &text, const QString &projectName)
{
    Project project;
    project.name = projectName;

    QStringList headerLines;
    QStringList footerLines;
    Column *currentColumn = nullptr;
    bool inFooter = false;

    // Description of the most recent task: indented lines below it, with
    // blank lines kept only when more indented content follows them.
    Task *currentTask = nullptr;
    QStringList descriptionLines;
    int pendingBlankLines = 0;

    auto finishTask = [&] {
        if (currentTask && !descriptionLines.isEmpty())
            currentTask->description = dedentDescription(descriptionLines);
        currentTask = nullptr;
        descriptionLines.clear();
        pendingBlankLines = 0;
    };

    const QStringList lines = text.split('\n');
    for (QString line : lines) {
        if (line.endsWith('\r'))
            line.chop(1);

        if (inFooter) {
            footerLines.append(line);
            continue;
        }

        if (line.startsWith("## ")) {
            // Start a new column
            finishTask();
            project.columns.append(Column{line.mid(3).trimmed(), {}});
            currentColumn = &project.columns.last();
        } else if (!currentColumn) {
            headerLines.append(line);
        } else if (currentTask && line.trimmed().isEmpty()) {
            ++pendingBlankLines;
        } else if (currentTask && (line.startsWith(' ') || line.startsWith('\t'))) {
            if (!descriptionLines.isEmpty())
                for (; pendingBlankLines > 0; --pendingBlankLines)
                    descriptionLines.append(QString());
            pendingBlankLines = 0;
            descriptionLines.append(line);
        } else if (line.trimmed() == "***" || line.startsWith("%%")) {
            // Obsidian archive separator / settings block: keep the rest as-is
            finishTask();
            inFooter = true;
            footerLines.append(line);
        } else {
            finishTask();
            Task task;
            if (parseTaskLine(line, task)) {
                currentColumn->tasks.append(task);
                currentTask = &currentColumn->tasks.last();
            }
        }
    }
    finishTask();

    project.header = trimTrailing(headerLines.join('\n'));
    project.footer = trimTrailing(footerLines.join('\n'));
    return project;
}

QString MarkdownIO::serialize(const Project &project)
{
    QString out;

    if (!project.header.isEmpty())
        out += project.header + "\n\n";

    for (int i = 0; i < project.columns.size(); ++i) {
        const Column &col = project.columns[i];

        out += "## " + col.name + "\n\n";

        for (const Task &task : col.tasks) {
            out += (task.done ? "- [x] " : "- [ ] ") + task.title;
            if (task.priority != Priority::None)
                out += " [priority:: " + priorityName(task.priority) + "]";
            if (!task.color.isEmpty())
                out += " [color:: " + task.color + "]";
            out += "\n";

            // Indented so Markdown (and this parser) nest it under the task.
            if (!task.description.trimmed().isEmpty()) {
                const QStringList descLines = trimTrailing(task.description).split('\n');
                for (const QString &line : descLines)
                    out += line.trimmed().isEmpty() ? QString("\n") : "    " + trimTrailing(line) + "\n";
            }
        }

        // Blank line between columns
        if (i < project.columns.size() - 1)
            out += "\n";
    }

    if (!project.footer.isEmpty()) {
        if (!out.isEmpty())
            out += "\n";   // columns already end with a newline
        out += project.footer + "\n";
    }

    return out;
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
        QFile file(d.filePath(filename));
        if (!file.open(QIODevice::ReadOnly))
            continue;

        // completeBaseName() keeps dots: "v1.2 plan.md" → "v1.2 plan"
        const QString name = QFileInfo(filename).completeBaseName();
        QString text = QString::fromUtf8(file.readAll());
        if (text.startsWith(QChar(0xFEFF)))   // UTF-8 BOM
            text.remove(0, 1);
        projects.append(parse(text, name));
    }

    return projects;
}

bool MarkdownIO::save(const QString &dir, const Project &project)
{
    if (!QDir().mkpath(dir))
        return false;

    // QSaveFile writes to a temp file and renames on commit, so a crash or a
    // full disk never leaves a truncated board behind.
    QSaveFile file(filePath(dir, project.name));
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(serialize(project).toUtf8());
    return file.commit();
}

bool MarkdownIO::remove(const QString &dir, const QString &projectName)
{
    return QFile::remove(filePath(dir, projectName));
}

bool MarkdownIO::rename(const QString &dir, const QString &oldName, const QString &newName)
{
    const QString oldPath = filePath(dir, oldName);
    if (!QFile::exists(oldPath))
        return true;
    return QFile::rename(oldPath, filePath(dir, newName));
}
