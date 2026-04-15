#pragma once

#include <QPoint>
#include <QWidget>
#include "model/appstate.h"

class QLabel;
class QVBoxLayout;
class QDragEnterEvent;
class QDropEvent;

/**
 * Displays one Kanban column: a header, an "Add Task" button, and a list of cards.
 *
 * The header (ColumnHeader) can be dragged to reorder columns.
 * Task cards can be dropped anywhere on the column.
 *
 * Communicates upward exclusively through signals — no pointer to BoardView.
 */
class ColumnWidget : public QWidget
{
    Q_OBJECT

public:
    ColumnWidget(AppState &state, int projectIdx, int colIdx, QWidget *parent = nullptr);

signals:
    void renameRequested(int colIdx, const QString &newName);
    void deleteRequested(int colIdx);
    void moveLeftRequested(int colIdx);
    void moveRightRequested(int colIdx);
    void columnMoveRequested(int fromColIdx, int toColIdx);

    void taskAdded(int colIdx, const QString &title);
    void taskDeleted(int colIdx, int taskIdx);
    void taskMoved(int fromColIdx, int taskIdx, int toColIdx);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    /** Forwards drag-and-drop events from child widgets (e.g. the Add Task button). */
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void showHeaderContextMenu(const QPoint &pos);
    void addTaskDialog();

private:
    void buildCards();

    AppState &m_state;
    int m_projectIdx;
    int m_colIdx;

    QLabel      *m_headerLabel = nullptr;
    QVBoxLayout *m_cardsLayout = nullptr;
};
