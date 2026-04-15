#pragma once

#include <QString>
#include <QList>

// ─── Core data types ──────────────────────────────────────────────────────────
//
// Plain value structs — no Qt parent, no signals. All mutation goes through
// AppState so that persistence is always kept in sync.

struct Task {
    QString title;
};

struct Column {
    QString name;
    QList<Task> tasks;
};

struct Project {
    QString name;
    QList<Column> columns;
};
