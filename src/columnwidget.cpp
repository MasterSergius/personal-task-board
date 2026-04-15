#include "columnwidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>

#include "cardwidget.h"

ColumnWidget::ColumnWidget(AppState &state, int projectIdx, int colIdx, QWidget *parent)
    : QWidget(parent)
    , m_state(state)
    , m_projectIdx(projectIdx)
    , m_colIdx(colIdx)
{
    setFixedWidth(240);
    setAcceptDrops(true);
    setStyleSheet("QWidget { background: #e8e8e8; border-radius: 8px; }");

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(8, 8, 8, 8);
    outerLayout->setSpacing(6);

    // ── Header ────────────────────────────────────────────────────────────────
    const QString &name = m_state.project(projectIdx).columns[colIdx].name;
    m_headerLabel = new QLabel(name);
    m_headerLabel->setStyleSheet(
        "font-weight: bold; font-size: 14px; background: transparent; border: none;");
    m_headerLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_headerLabel, &QLabel::customContextMenuRequested,
            this, &ColumnWidget::showHeaderContextMenu);

    // ── Add Task button ───────────────────────────────────────────────────────
    auto *addBtn = new QPushButton("+ Add Task");
    addBtn->setStyleSheet(
        "QPushButton { text-align: left; background: transparent; border: none; color: #555; }"
        "QPushButton:hover { color: #000; }");
    connect(addBtn, &QPushButton::clicked, this, &ColumnWidget::addTaskDialog);

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
    // Clear existing card widgets
    while (m_cardsLayout->count() > 0) {
        auto *item = m_cardsLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    const auto &tasks = m_state.project(m_projectIdx).columns[m_colIdx].tasks;
    for (int i = 0; i < tasks.size(); ++i) {
        auto *card = new CardWidget(tasks[i].title, m_projectIdx, m_colIdx, i);

        // Route the card's delete request up to BoardView via our own signal
        connect(card, &CardWidget::deleteRequested,
                this, [this](int colIdx, int taskIdx) {
                    emit taskDeleted(colIdx, taskIdx);
                });

        m_cardsLayout->addWidget(card);
    }
}

// ─── Header context menu ──────────────────────────────────────────────────────

void ColumnWidget::showHeaderContextMenu(const QPoint &pos)
{
    const auto &cols = m_state.project(m_projectIdx).columns;

    QMenu menu(this);
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
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void ColumnWidget::dropEvent(QDropEvent *event)
{
    const QStringList parts = event->mimeData()->text().split(':');
    if (parts.size() != 2)
        return;

    bool ok1, ok2;
    int fromColIdx = parts[0].toInt(&ok1);
    int taskIdx    = parts[1].toInt(&ok2);

    if (!ok1 || !ok2)
        return;

    event->acceptProposedAction();

    if (fromColIdx != m_colIdx)
        emit taskMoved(fromColIdx, taskIdx, m_colIdx);
    // Same-column drop: just let BoardView's deferred refresh restore the card
}
