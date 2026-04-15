#include "mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QSplitter>
#include <QWidget>

#include "boardview.h"

// ─── Startup ──────────────────────────────────────────────────────────────────

QString MainWindow::boardsDirectory()
{
    return QDir::homePath() + "/.ptb/projects";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_state(boardsDirectory())
{
    m_state.load();

    if (m_state.projectCount() == 0)
        seedSampleData();

    setupUI();
}

void MainWindow::seedSampleData()
{
    int idx = m_state.addProject("Sample Project");
    m_state.addColumn(idx, "To Do");
    m_state.addColumn(idx, "In Progress");
    m_state.addColumn(idx, "Done");
    m_state.addTask(idx, 0, "Create your first task");
    m_state.addTask(idx, 0, "Right-click a card to delete it");
    m_state.addTask(idx, 1, "Drag cards between columns");
}

// ─── UI setup ─────────────────────────────────────────────────────────────────

void MainWindow::setupUI()
{
    setWindowTitle("Personal Task Board");
    resize(1200, 700);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal, central);

    // ── Left: project list ────────────────────────────────────────────────────
    auto *sidePanel = new QWidget();
    sidePanel->setMaximumWidth(220);
    sidePanel->setMinimumWidth(140);
    sidePanel->setStyleSheet("background: #2d2d2d;");

    auto *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 8, 0, 8);
    sideLayout->setSpacing(0);

    auto *sideTitle = new QLabel("  Projects");
    sideTitle->setStyleSheet("color: #aaa; font-size: 11px; font-weight: bold; padding: 4px 8px;");
    sideLayout->addWidget(sideTitle);

    m_projectList = new QListWidget();
    m_projectList->setStyleSheet(R"(
        QListWidget {
            background: transparent;
            border: none;
            color: #ddd;
        }
        QListWidget::item {
            padding: 6px 12px;
        }
        QListWidget::item:selected {
            background: #4a4a4a;
            color: #fff;
            border-radius: 4px;
        }
        QListWidget::item:hover:!selected {
            background: #3a3a3a;
        }
    )");
    m_projectList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_projectList, &QListWidget::currentRowChanged,
            this, &MainWindow::onProjectSelectionChanged);
    connect(m_projectList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showProjectContextMenu);

    sideLayout->addWidget(m_projectList);

    // ── Right: board view ─────────────────────────────────────────────────────
    m_board = new BoardView(m_state, this);

    m_splitter->addWidget(sidePanel);
    m_splitter->addWidget(m_board);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({180, 1020});

    layout->addWidget(m_splitter);

    refreshProjectList(0);
}

// ─── Project list ─────────────────────────────────────────────────────────────

void MainWindow::refreshProjectList(int selectRow)
{
    // Block signals so setCurrentRow doesn't fire onProjectSelectionChanged twice
    m_projectList->blockSignals(true);
    m_projectList->clear();
    m_projectList->addItems(m_state.projectNames());
    m_projectList->blockSignals(false);

    if (selectRow >= 0 && selectRow < m_state.projectCount())
        m_projectList->setCurrentRow(selectRow);
    else if (m_state.projectCount() > 0)
        m_projectList->setCurrentRow(0);
}

void MainWindow::onProjectSelectionChanged(int row)
{
    if (row >= 0 && row < m_state.projectCount())
        m_board->loadProject(row);
    else
        m_board->clear();
}

// ─── Project context menu ─────────────────────────────────────────────────────

void MainWindow::showProjectContextMenu(const QPoint &pos)
{
    static const char *kMenuStyle =
        "QMenu { background: #ffffff; color: #111111; border: 1px solid #cccccc; }"
        "QMenu::item:selected { background: #0078d4; color: #ffffff; }"
        "QMenu::separator { height: 1px; background: #cccccc; margin: 4px 8px; }";

    QListWidgetItem *item = m_projectList->itemAt(pos);

    QMenu menu(this);
    menu.setStyleSheet(kMenuStyle);

    if (item) {
        // Right-clicked on a specific project: select it first so rename/delete
        // operate on the intended row, not whatever was previously highlighted.
        m_projectList->setCurrentItem(item);

        QAction *renameAction = menu.addAction("Rename Project…");
        QAction *deleteAction = menu.addAction("Delete Project");
        menu.addSeparator();
        QAction *addAction    = menu.addAction("Add Project…");

        QAction *selected = menu.exec(m_projectList->mapToGlobal(pos));
        if      (selected == renameAction) renameProjectDialog();
        else if (selected == deleteAction) deleteCurrentProject();
        else if (selected == addAction)    addProjectDialog();
    } else {
        // Right-clicked on empty space: only offer adding a new project.
        QAction *addAction = menu.addAction("Add Project…");
        if (menu.exec(m_projectList->mapToGlobal(pos)) == addAction)
            addProjectDialog();
    }
}

// ─── Dialogs ──────────────────────────────────────────────────────────────────

void MainWindow::addProjectDialog()
{
    bool ok;
    QString name = QInputDialog::getText(
        this, "Add Project", "Project name:", QLineEdit::Normal, "", &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    int idx = m_state.addProject(name.trimmed());
    if (idx < 0) {
        QMessageBox::warning(this, "Add Project",
            QString("A project named \"%1\" already exists.").arg(name.trimmed()));
        return;
    }

    // New projects get default columns so the board isn't blank
    m_state.addColumn(idx, "To Do");
    m_state.addColumn(idx, "In Progress");
    m_state.addColumn(idx, "Done");

    refreshProjectList(idx);
}

void MainWindow::renameProjectDialog()
{
    int row = m_projectList->currentRow();
    if (row < 0) return;

    const QString oldName = m_state.project(row).name;

    bool ok;
    QString newName = QInputDialog::getText(
        this, "Rename Project", "New name:",
        QLineEdit::Normal, oldName, &ok);

    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == oldName)
        return;

    if (!m_state.renameProject(row, newName.trimmed())) {
        QMessageBox::warning(this, "Rename Project",
            QString("A project named \"%1\" already exists.").arg(newName.trimmed()));
        return;
    }

    refreshProjectList(row);
}

void MainWindow::deleteCurrentProject()
{
    int row = m_projectList->currentRow();
    if (row < 0) return;

    const QString name = m_state.project(row).name;
    auto reply = QMessageBox::question(
        this, "Delete Project",
        QString("Delete project \"%1\" and its .md file?\nThis cannot be undone.").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    m_state.removeProject(row);

    int nextRow = qMin(row, m_state.projectCount() - 1);
    refreshProjectList(nextRow);

    if (m_state.projectCount() == 0)
        m_board->clear();
}
