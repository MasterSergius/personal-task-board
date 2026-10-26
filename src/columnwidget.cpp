#include "columnwidget.h"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include "cardwidget.h"

// ─── ColumnHeader ─────────────────────────────────────────────────────────────
//
// A QLabel subclass that initiates a column-reorder drag when the user
// clicks and drags. Uses the private MIME type ColumnHeader::MimeType.

class ColumnHeader : public QLabel
{
    Q_OBJECT

public:
    static constexpr const char *MimeType = "application/x-ptb-column";

    ColumnHeader(const QString &text, int colIdx, QWidget *parent = nullptr)
        : QLabel(text, parent), m_colIdx(colIdx)
    {
        setCursor(Qt::SizeHorCursor);
    }

    /** Called by the drop target while the drag is in progress. */
    void setDropColumn(int colIdx) { m_dropColIdx = colIdx; }

signals:
    /** Emitted after QDrag::exec() returns from a successful drop. */
    void moveRequested(int fromColIdx, int toColIdx);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            m_dragStartPos = event->pos();
        QLabel::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!(event->buttons() & Qt::LeftButton) || m_dragStartPos.isNull())
            return;
        if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
            return;

        // Reset before exec() to prevent re-entrancy
        const QPoint hotSpot = m_dragStartPos;
        m_dragStartPos = QPoint();
        m_dropColIdx   = -1;

        auto *drag = new QDrag(this);
        auto *mime = new QMimeData();
        mime->setData(MimeType, QByteArray());
        drag->setMimeData(mime);
        drag->setPixmap(grab());
        drag->setHotSpot(hotSpot);

        // Emit only after exec() returns — see CardWidget::mouseMoveEvent.
        if (drag->exec(Qt::MoveAction) == Qt::MoveAction
            && m_dropColIdx >= 0 && m_dropColIdx != m_colIdx)
            emit moveRequested(m_colIdx, m_dropColIdx);
    }

private:
    int    m_colIdx;
    int    m_dropColIdx = -1;
    QPoint m_dragStartPos;
};

// Required for Q_OBJECT defined in a .cpp file
#include "columnwidget.moc"

// ─── ColumnWidget ─────────────────────────────────────────────────────────────

ColumnWidget::ColumnWidget(AppState &state, int projectIdx, int colIdx,
                           const Theme &theme, QWidget *parent)
    : QWidget(parent)
    , m_state(state)
    , m_projectIdx(projectIdx)
    , m_colIdx(colIdx)
    , m_theme(theme)
{
    setFixedWidth(240);
    setAcceptDrops(true);
    setStyleSheet(theme.columnStyle);
    setObjectName("column");

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(8, 8, 8, 8);
    outerLayout->setSpacing(6);

    // ── Header (drag handle for column reordering) ────────────────────────────
    const QString &name = m_state.project(projectIdx).columns[colIdx].name;
    m_headerLabel = new ColumnHeader(name, colIdx, this);
    m_headerLabel->setStyleSheet(theme.columnHeaderStyle);
    m_headerLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_headerLabel, &QLabel::customContextMenuRequested,
            this, &ColumnWidget::showHeaderContextMenu);
    connect(static_cast<ColumnHeader *>(m_headerLabel), &ColumnHeader::moveRequested,
            this, &ColumnWidget::columnMoveRequested);

    // ── Add Task button ───────────────────────────────────────────────────────
    auto *addBtn = new QPushButton("+ Add Task");
    addBtn->setStyleSheet(theme.addTaskBtnStyle);
    connect(addBtn, &QPushButton::clicked, this, &ColumnWidget::addTaskDialog);

    // Forward drag events from the button so drops on it reach this column
    addBtn->installEventFilter(this);

    // ── Cards area ────────────────────────────────────────────────────────────
    auto *cardsContainer = new QWidget();
    cardsContainer->setStyleSheet("background: transparent;");
    m_cardsLayout = new QVBoxLayout(cardsContainer);
    m_cardsLayout->setSpacing(6);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);

    outerLayout->addWidget(m_headerLabel);
    outerLayout->addWidget(addBtn);
    outerLayout->addWidget(cardsContainer);
    outerLayout->addStretch();

    buildCards();
}

// ─── Cards ────────────────────────────────────────────────────────────────────

void ColumnWidget::buildCards()
{
    while (m_cardsLayout->count() > 0) {
        auto *item = m_cardsLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    const auto &tasks = m_state.project(m_projectIdx).columns[m_colIdx].tasks;
    for (int i = 0; i < tasks.size(); ++i) {
        auto *card = new CardWidget(tasks[i], m_projectIdx, m_colIdx, i, m_theme);
        connect(card, &CardWidget::deleteRequested,
                this, [this](int colIdx, int taskIdx) {
                    emit taskDeleted(colIdx, taskIdx);
                });
        connect(card, &CardWidget::moveRequested, this, &ColumnWidget::taskMoved);
        connect(card, &CardWidget::priorityChangeRequested,
                this, &ColumnWidget::taskPriorityChanged);
        connect(card, &CardWidget::colorChangeRequested,
                this, &ColumnWidget::taskColorChanged);
        m_cardsLayout->addWidget(card);
    }
}

// ─── Event filter (forwards drops from Add Task button to this column) ────────

bool ColumnWidget::eventFilter(QObject * /*watched*/, QEvent *event)
{
    if (event->type() == QEvent::DragEnter) {
        dragEnterEvent(static_cast<QDragEnterEvent *>(event));
        return true;
    }
    if (event->type() == QEvent::Drop) {
        dropEvent(static_cast<QDropEvent *>(event));
        return true;
    }
    return false;
}

// ─── Header context menu ──────────────────────────────────────────────────────

void ColumnWidget::showHeaderContextMenu(const QPoint &pos)
{
    const auto &cols = m_state.project(m_projectIdx).columns;

    QMenu menu(this);
    menu.setStyleSheet(m_theme.menuStyle);
    QAction *renameAction    = menu.addAction("Rename Column");
    QAction *moveLeftAction  = menu.addAction("Move Left");
    QAction *moveRightAction = menu.addAction("Move Right");
    menu.addSeparator();
    QAction *deleteAction    = menu.addAction("Delete Column");

    moveLeftAction->setEnabled(m_colIdx > 0);
    moveRightAction->setEnabled(m_colIdx < cols.size() - 1);

    QAction *selected = menu.exec(m_headerLabel->mapToGlobal(pos));

    if (selected == renameAction) {
        bool ok;
        QString newName = QInputDialog::getText(
            this, "Rename Column", "New name:",
            QLineEdit::Normal, cols[m_colIdx].name, &ok);
        if (ok && !newName.trimmed().isEmpty())
            emit renameRequested(m_colIdx, newName.trimmed());

    } else if (selected == moveLeftAction) {
        emit moveLeftRequested(m_colIdx);

    } else if (selected == moveRightAction) {
        emit moveRightRequested(m_colIdx);

    } else if (selected == deleteAction) {
        auto reply = QMessageBox::question(
            this, "Delete Column",
            QString("Delete column \"%1\" and all its tasks?\nThis cannot be undone.")
                .arg(cols[m_colIdx].name),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
            emit deleteRequested(m_colIdx);
    }
}

// ─── Add task ─────────────────────────────────────────────────────────────────

void ColumnWidget::addTaskDialog()
{
    bool ok;
    QString title = QInputDialog::getText(
        this, "Add Task", "Task title:", QLineEdit::Normal, "", &ok);
    if (ok && !title.trimmed().isEmpty())
        emit taskAdded(m_colIdx, title.trimmed());
}

// ─── Drag & drop ─────────────────────────────────────────────────────────────

void ColumnWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // Only accept drags started by this application's cards / column headers.
    // event->source() is null for drags coming from other applications.
    const QMimeData *mime = event->mimeData();
    if ((mime->hasFormat(CardWidget::MimeType)   && qobject_cast<CardWidget *>(event->source()))
     || (mime->hasFormat(ColumnHeader::MimeType) && qobject_cast<ColumnHeader *>(event->source())))
        event->acceptProposedAction();
}

void ColumnWidget::dropEvent(QDropEvent *event)
{
    // Only record the target here. The source emits the move request once
    // QDrag::exec() has returned, so the board is not rebuilt (and the source
    // widget deleted) while the drag is still running.

    if (auto *header = qobject_cast<ColumnHeader *>(event->source());
        header && event->mimeData()->hasFormat(ColumnHeader::MimeType)) {
        header->setDropColumn(m_colIdx);
        event->acceptProposedAction();
        return;
    }

    if (auto *card = qobject_cast<CardWidget *>(event->source());
        card && event->mimeData()->hasFormat(CardWidget::MimeType)) {
        card->setDropColumn(m_colIdx);
        event->acceptProposedAction();
    }
}
