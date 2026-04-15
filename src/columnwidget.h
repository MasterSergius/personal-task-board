#pragma once

#include <QWidget>
#include "model/appstate.h"

class QLabel;
class QVBoxLayout;
class QDragEnterEvent;
class QDropEvent;

/**
 * Displays one Kanban column: a header, an "Add Task" button, and a list of cards.
 *
 * Communicates upward exclusively through signals — no pointer to BoardView.
 * BoardView connects to these signals and updates the model + triggers a refresh.
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

    void taskAdded(int colIdx, const QString &title);
    void taskDeleted(int colIdx, int taskIdx);
    void taskMoved(int fromColIdx, int taskIdx, int toColIdx);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void showHeaderContextMenu(const QPoint &pos);
    void addTaskDialog();

private:
    /** (Re)build the card widgets from current model data. */
    void buildCards();

    AppState &m_state;
    int m_projectIdx;
    int m_colIdx;

    QLabel     *m_headerLabel = nullptr;
    QVBoxLayout *m_cardsLayout = nullptr;
};
