#include "mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
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
    setupUI();
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
    m_sidePanel = new QWidget();
    m_sidePanel->setMaximumWidth(220);
    m_sidePanel->setMinimumWidth(140);

    auto *sideLayout = new QVBoxLayout(m_sidePanel);
    sideLayout->setContentsMargins(0, 8, 0, 8);
    sideLayout->setSpacing(0);

    // Header row: "Projects" label + theme toggle button
    auto *headerRow = new QWidget();
    auto *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 4, 0);
    headerLayout->setSpacing(0);

    m_sideTitle = new QLabel("  Projects");
    headerLayout->addWidget(m_sideTitle, 1);

    m_toggleBtn = new QPushButton("Dark");
    m_toggleBtn->setFixedHeight(20);
    m_toggleBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(m_toggleBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    headerLayout->addWidget(m_toggleBtn);

    sideLayout->addWidget(headerRow);

    m_projectList = new QListWidget();
    m_projectList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_projectList, &QListWidget::currentRowChanged,
            this, &MainWindow::onProjectSelectionChanged);
    connect(m_projectList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showProjectContextMenu);

    sideLayout->addWidget(m_projectList);

    // ── Right: board view ─────────────────────────────────────────────────────
    m_board = new BoardView(m_state, this);

    m_splitter->addWidget(m_sidePanel);
    m_splitter->addWidget(m_board);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({180, 1020});

    layout->addWidget(m_splitter);

    applyTheme();
    refreshProjectList(0);
}

// ─── Theme ────────────────────────────────────────────────────────────────────

void MainWindow::applyTheme()
{
    m_sidePanel->setStyleSheet(m_theme.sidebarBg);
    m_sideTitle->setStyleSheet(m_theme.sidebarTitleStyle);
    m_projectList->setStyleSheet(m_theme.projectListStyle);
    m_toggleBtn->setText(m_theme.isDark ? "Light" : "Dark");
    m_toggleBtn->setStyleSheet(m_theme.toggleBtnStyle);
    m_board->applyTheme(m_theme);
}

void MainWindow::toggleTheme()
{
    m_theme = m_theme.isDark ? Theme::light() : Theme::dark();
    applyTheme();
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
    QListWidgetItem *item = m_projectList->itemAt(pos);

    QMenu menu(this);
    menu.setStyleSheet(m_theme.menuStyle);

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
