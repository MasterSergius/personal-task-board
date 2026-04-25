#pragma once

#include <QFrame>
#include <QPoint>
#include "theme.h"

class QLabel;
class QMouseEvent;
class QContextMenuEvent;

/**
 * A single task card on the Kanban board.
 *
 * Supports:
 *  - Drag to move between columns (MIME text = "colIdx:taskIdx")
 *  - Right-click context menu to delete the task
 *
 * The card carries its own position indices so the drop target can update
 * the model without needing a reference to the source column.
 */
class CardWidget : public QFrame
{
    Q_OBJECT

public:
    CardWidget(const QString &title, int projectIdx, int colIdx, int taskIdx,
               const Theme &theme, QWidget *parent = nullptr);

signals:
    /** Emitted when the user confirms deletion via context menu. */
    void deleteRequested(int colIdx, int taskIdx);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    int m_projectIdx;
    int m_colIdx;
    int m_taskIdx;

    QLabel  *m_label        = nullptr;
    QPoint   m_dragStartPos;
    Theme    m_theme;
};
