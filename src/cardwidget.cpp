#include "cardwidget.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDrag>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QVBoxLayout>

CardWidget::CardWidget(const QString &title, int projectIdx, int colIdx, int taskIdx,
                       QWidget *parent)
    : QFrame(parent)
    , m_projectIdx(projectIdx)
    , m_colIdx(colIdx)
    , m_taskIdx(taskIdx)
{
    setStyleSheet(R"(
        QFrame {
            border: 1px solid #b0b0b0;
            border-radius: 6px;
            background: #fafafa;
        }
        QFrame:hover {
            background: #f0f0f0;
            border-color: #888;
        }
    )");
    setCursor(Qt::OpenHandCursor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);

    m_label = new QLabel(title);
    m_label->setWordWrap(true);
    // Labels must be transparent so the frame's stylesheet shows through
    m_label->setStyleSheet("border: none; background: transparent;");
    layout->addWidget(m_label);
}

// ─── Drag ─────────────────────────────────────────────────────────────────────

void CardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
}

void CardWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_dragStartPos.isNull())
        return;

    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
        return;

    auto *drag     = new QDrag(this);
    auto *mimeData = new QMimeData();

    // Encode source position as "colIdx:taskIdx" so the drop target can update the model
    mimeData->setText(QString("%1:%2").arg(m_colIdx).arg(m_taskIdx));
    drag->setMimeData(mimeData);

    // Snapshot the card as the drag cursor image
    drag->setPixmap(grab());
    drag->setHotSpot(m_dragStartPos);

    // Hide original while dragging; it will be rebuilt by BoardView::refresh()
    hide();

    Qt::DropAction result = drag->exec(Qt::MoveAction);

    // If the drop was cancelled the board is NOT refreshed, so we must reappear
    if (result != Qt::MoveAction)
        show();
    // On success: BoardView::refresh() (deferred via QTimer) recreates all cards

    m_dragStartPos = QPoint();
}

// ─── Context menu ─────────────────────────────────────────────────────────────

void CardWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *deleteAction = menu.addAction("Delete Task");

    if (menu.exec(event->globalPos()) == deleteAction) {
        auto reply = QMessageBox::question(
            this, "Delete Task",
            QString("Delete task \"%1\"?").arg(m_label->text()),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
            emit deleteRequested(m_colIdx, m_taskIdx);
    }
}
