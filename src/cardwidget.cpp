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
                       const Theme &theme, QWidget *parent)
    : QFrame(parent)
    , m_projectIdx(projectIdx)
    , m_colIdx(colIdx)
    , m_taskIdx(taskIdx)
    , m_theme(theme)
{
    setStyleSheet(theme.cardStyle);
    setCursor(Qt::OpenHandCursor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);

    m_label = new QLabel(title);
    m_label->setWordWrap(true);
    m_label->setStyleSheet(theme.cardLabelStyle);
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

    const QPoint hotSpot = m_dragStartPos;
    m_dragStartPos = QPoint();
    m_dropColIdx   = -1;

    auto *drag     = new QDrag(this);
    auto *mimeData = new QMimeData();

    // Private MIME type: the drop target only accepts drags from this app.
    // Indices travel via the source widget, not the payload.
    mimeData->setData(MimeType, QByteArray());
    drag->setMimeData(mimeData);

    // Snapshot the card as the drag cursor image
    drag->setPixmap(grab());
    drag->setHotSpot(hotSpot);

    // Hide original while dragging
    hide();

    const Qt::DropAction result = drag->exec(Qt::MoveAction);

    // The model update happens here, after exec() has returned, rather than in
    // the target's dropEvent(): rebuilding the board deletes this card, and
    // doing that while still inside the drag's nested event loop would leave
    // exec() running on a destroyed QDrag.
    if (result == Qt::MoveAction && m_dropColIdx >= 0 && m_dropColIdx != m_colIdx) {
        emit moveRequested(m_colIdx, m_taskIdx, m_dropColIdx);   // board rebuild follows
        return;
    }

    show();
}

// ─── Context menu ─────────────────────────────────────────────────────────────

void CardWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.setStyleSheet(m_theme.menuStyle);
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
