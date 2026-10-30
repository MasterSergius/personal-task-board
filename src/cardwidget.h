#pragma once

#include <QFrame>
#include <QPoint>
#include "model/types.h"
#include "theme.h"

class QLabel;
class QMouseEvent;
class QContextMenuEvent;

/**
 * A single task card on the Kanban board.
 *
 * Supports:
 *  - Drag to move between columns (MIME type CardWidget::MimeType)
 *  - Priority badge and custom frame color
 *  - Description preview and check-list progress
 *  - Double-click (or "Open…" in the context menu) to open the task editor
 *  - Right-click context menu to set priority / frame color or delete the task
 *
 * The card carries its own position indices so the drop target can update
 * the model without needing a reference to the source column.
 */
class CardWidget : public QFrame
{
    Q_OBJECT

public:
    CardWidget(const Task &task, int projectIdx, int colIdx, int taskIdx,
               const Theme &theme, QWidget *parent = nullptr);

    static constexpr const char *MimeType = "application/x-ptb-task";

    /** Preset frame colors offered in the context menu: {label, "#rrggbb"}. */
    static const QList<QPair<QString, QString>> &framePalette();

    /** Badge text and background color for a priority (empty for None). */
    static QString priorityLabel(Priority priority);
    static QString priorityColor(Priority priority);

    /** Plain-text summary of a Markdown description, at most maxLength characters. */
    static QString descriptionPreview(const QString &markdown, int maxLength = 120);

    /** {checked, total} check-list items ("- [ ]" / "- [x]") in a description. */
    static QPair<int, int> checkListProgress(const QString &markdown);

    /** Called by the drop target while the drag is in progress. */
    void setDropColumn(int colIdx) { m_dropColIdx = colIdx; }

signals:
    /** Emitted when the user confirms deletion via context menu. */
    void deleteRequested(int colIdx, int taskIdx);

    /**
     * Emitted after a successful drop on another column. Sent only once
     * QDrag::exec() has returned, so receivers may safely rebuild the board.
     */
    void moveRequested(int fromColIdx, int taskIdx, int toColIdx);

    void priorityChangeRequested(int colIdx, int taskIdx, Priority priority);

    /** color is "#rrggbb", or empty to reset to the theme default. */
    void colorChangeRequested(int colIdx, int taskIdx, const QString &color);

    /** Emitted on double-click or "Open…": the task editor should be shown. */
    void openRequested(int colIdx, int taskIdx);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    int m_projectIdx;
    int m_colIdx;
    int m_taskIdx;
    int m_dropColIdx = -1;

    Priority m_priority;
    QString  m_color;

    QLabel  *m_badge        = nullptr;
    QLabel  *m_label        = nullptr;
    QPoint   m_dragStartPos;
    Theme    m_theme;
};
