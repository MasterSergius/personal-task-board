#include "boardview.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QPushButton>
#include <QScrollArea>
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

    m_container = new QWidget();
    m_container->setStyleSheet(m_theme.boardBg);
    auto *container = m_container;

    m_columnsLayout = new QHBoxLayout(container);
    m_columnsLayout->setSpacing(12);
    m_columnsLayout->setContentsMargins(16, 16, 16, 16);
    m_columnsLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // "Add Column" button — always lives at the trailing edge of the board.
    // It is re-added to the layout on every refresh() rather than being
    // deleted, so we give it the container as parent to keep it alive.
    m_addColumnBtn = new QPushButton("+ Add Column", container);
    m_addColumnBtn->setFixedWidth(160);
    m_addColumnBtn->setStyleSheet(m_theme.addColumnBtnStyle);
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

void BoardView::applyTheme(const Theme &theme)
{
    m_theme = theme;
    m_container->setStyleSheet(theme.boardBg);
    m_addColumnBtn->setStyleSheet(theme.addColumnBtnStyle);
    refresh();
}

// ─── Rebuild ──────────────────────────────────────────────────────────────────

void BoardView::refresh()
{
    // Remove all items from the layout. The add-column button is kept alive
    // (it belongs to the container, not the layout) and re-added at the end.
    while (m_columnsLayout->count() > 0) {
        auto *item = m_columnsLayout->takeAt(0);
        QWidget *w = item->widget();
        if (w && w != m_addColumnBtn)
            w->deleteLater();
        delete item;
    }

    if (m_projectIdx >= 0) {
        const Project &proj = m_state.project(m_projectIdx);

        for (int i = 0; i < proj.columns.size(); ++i) {
            auto *col = new ColumnWidget(m_state, m_projectIdx, i, m_theme);

            // Column-level operations — model is updated, then board is rebuilt.
            // refresh() is called directly (not deferred) so the new column
            // widgets are in place before drag->exec() returns to the card.
            connect(col, &ColumnWidget::renameRequested,
                    this, [this](int colIdx, const QString &name) {
                        m_state.renameColumn(m_projectIdx, colIdx, name);
                        refresh();
                    });
            connect(col, &ColumnWidget::deleteRequested,
                    this, [this](int colIdx) {
                        m_state.removeColumn(m_projectIdx, colIdx);
                        refresh();
                    });
            connect(col, &ColumnWidget::moveLeftRequested,
                    this, [this](int colIdx) {
                        m_state.moveColumnLeft(m_projectIdx, colIdx);
                        refresh();
                    });
            connect(col, &ColumnWidget::moveRightRequested,
                    this, [this](int colIdx) {
                        m_state.moveColumnRight(m_projectIdx, colIdx);
                        refresh();
                    });
            connect(col, &ColumnWidget::columnMoveRequested,
                    this, [this](int fromColIdx, int toColIdx) {
                        m_state.moveColumn(m_projectIdx, fromColIdx, toColIdx);
                        refresh();
                    });

            // Task-level operations
            connect(col, &ColumnWidget::taskAdded,
                    this, [this](int colIdx, const QString &title) {
                        m_state.addTask(m_projectIdx, colIdx, title);
                        refresh();
                    });
            connect(col, &ColumnWidget::taskDeleted,
                    this, [this](int colIdx, int taskIdx) {
                        m_state.removeTask(m_projectIdx, colIdx, taskIdx);
                        refresh();
                    });
            connect(col, &ColumnWidget::taskMoved,
                    this, [this](int fromColIdx, int taskIdx, int toColIdx) {
                        m_state.moveTask(m_projectIdx, fromColIdx, taskIdx, toColIdx);
                        refresh();
                    });

            m_columnsLayout->addWidget(col);
        }
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
