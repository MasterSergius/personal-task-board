#pragma once

#include <QString>
#include <QList>

// ─── Core data types ──────────────────────────────────────────────────────────
//
// Plain value structs — no Qt parent, no signals. All mutation goes through
// AppState so that persistence is always kept in sync.

enum class Priority { None, Low, Medium, High, Urgent };

struct Task {
    QString  title;
    bool     done     = false;            // "- [x]" in Markdown; preserved on save
    Priority priority = Priority::None;   // "[priority:: high]"
    QString  color;                       // "[color:: #e5484d]"; empty = theme default
    QString  description;                 // Markdown, stored indented below the task line
};

struct Column {
    QString name;
    QList<Task> tasks;
};

struct Project {
    QString name;
    QList<Column> columns;

    // Content the board UI does not edit, kept verbatim so that saving does
    // not destroy it (e.g. Obsidian Kanban's YAML front matter, its
    // "%% kanban:settings" block and the "***" archive section).
    QString header;   // everything before the first "## " heading
    QString footer;   // everything from the first "***" or "%%" line onward
};
