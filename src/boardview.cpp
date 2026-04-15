#include "boardview.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QWidget>

#include "columnwidget.h"

BoardView::BoardView(AppState &state, QWidget *parent)
    : QWidget(parent)
    , m_state(state)
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget();
    container->setStyleSheet("background: #d4d4d4;");

    m_columnsLayout = new QHBoxLayout(container);
    m_columnsLayout->setSpacing(12);
    m_columnsLayout->setContentsMargins(16, 16, 16, 16);
    m_columnsLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // "Add Column" button — always lives at the trailing edge of the board
    m_addColumnBtn = new QPushButton("+ Add Column");
    m_addColumnBtn->setFixedWidth(160);
    m_addColumnBtn->setStyleSheet(
        "QPushButton {"
        "  color: #666; border: 2px dashed #aaa; border-radius: 8px; padding: 10px;"
        "  background: transparent;"
        "}"
        "QPushButton:hover { color: #333; border-color: #777; }");
    connect(m_addColumnBtn, &QPushButton::clicked, this, &BoardView::onAddColumnClicked);

    scroll->setWidget(container);
    mainLayout->addWidget(scroll);
}

// ─── Project loading ──────────────────────────────────────────────────────────

void BoardView::loadProject(int projectIdx)
{
    m_projectIdx = projectIdx;
    refresh();
}

void BoardView::clear()
{
    m_projectIdx = -1;
    refresh();
}

// ─── Rebuild ──────────────────────────────────────────────────────────────────

void BoardView::refresh()
{
    // Remove all items from the layout without deleting m_addColumnBtn
    while (m_columnsLayout->count() > 0) {
        auto *item = m_columnsLayout->takeAt(0);
        QWidget *w = item->widget();
        if (w && w != m_addColumnBtn)
            w->deleteLater();
        delete item;
    }

    if (m_projectIdx < 0) {
        m_columnsLayout->addWidget(m_addColumnBtn);
        return;
    }

    const Project &proj = m_state.project(m_projectIdx);

    for (int i = 0; i < proj.columns.size(); ++i) {
        auto *col = new ColumnWidget(m_state, m_projectIdx, i);

        // Column-level operations: update model, then schedule a deferred refresh
        // (deferred so we don't destroy ColumnWidget while still inside its signal handler)
        connect(col, &ColumnWidget::renameRequested,
                this, [this](int colIdx, const QString &name) {
                    m_state.renameColumn(m_projectIdx, colIdx, name);
                    QTimer::singleShot(0, this, &BoardView::refresh);
                });

        connect(col, &ColumnWidget::deleteRequested,
                this, [this](int colIdx) {
                    m_state.removeColumn(m_projectIdx, colIdx);
                    QTimer::singleShot(0, this, &BoardView::refresh);
                });

        connect(col, &ColumnWidget::moveLeftRequested,
                this, [this](int colIdx) {
                    m_state.moveColumnLeft(m_projectIdx, colIdx);
                    QTimer::singleShot(0, this, &BoardView::refresh);
                });

        connect(col, &ColumnWidget::moveRightRequested,
                this, [this](int colIdx) {
                    m_state.moveColumnRight(m_projectIdx, colIdx);
                    QTimer::singleShot(0, this, &BoardView::refresh);
                });

        // Task-level operations
        connect(col, &ColumnWidget::taskAdded,
                this, [this](int colIdx, const QString &title) {
                    m_state.addTask(m_projectIdx, colIdx, title);
                    QTimer::singleShot(0, this, &BoardView::refresh);
                });

        connect(col, &ColumnWidget::taskDeleted,
                this, [this](int colIdx, int taskIdx) {
                    m_state.removeTask(m_projectIdx, colIdx, taskIdx);
                    QTimer::singleShot(0, this, &BoardView::refresh);
                });

        connect(col, &ColumnWidget::taskMoved,
                this, [this](int fromColIdx, int taskIdx, int toColIdx) {
                    m_state.moveTask(m_projectIdx, fromColIdx, taskIdx, toColIdx);
                    QTimer::singleShot(0, this, &BoardView::refresh);
                });

        m_columnsLayout->addWidget(col);
    }

    m_columnsLayout->addWidget(m_addColumnBtn);
}

// ─── Add column ───────────────────────────────────────────────────────────────

void BoardView::onAddColumnClicked()
{
    if (m_projectIdx < 0)
        return;

    bool ok;
    QString name = QInputDialog::getText(
        this, "Add Column", "Column name:", QLineEdit::Normal, "", &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    m_state.addColumn(m_projectIdx, name.trimmed());
    refresh();
}
