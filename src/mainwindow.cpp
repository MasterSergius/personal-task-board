#include "mainwindow.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QListWidget>
#include <QMenu>
#include <QSplitter>
#include <QWidget>

#include "boardview.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

void MainWindow::showProjectsContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    QAction *addAction = menu.addAction("Add Project");
    QAction *deleteAction = menu.addAction("Delete Project");

    QAction *selected = menu.exec(projectsList->mapToGlobal(pos));

    if (selected == addAction) {
        bool ok;
        QString name = QInputDialog::getText(this, "New Project", "Name:", QLineEdit::Normal, "", &ok);

        if (ok && !name.isEmpty()) {
            projectsList->addItem(name);
        }
    }

    if (selected == deleteAction) {
        auto *item = projectsList->currentItem();
        if (item) {
            delete item;
        }
    }
}

void MainWindow::setupUI()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QHBoxLayout(central);

    splitter = new QSplitter(Qt::Horizontal);

    projectsList = new QListWidget();
    projectsList->addItems({"Project A", "Project B"});
    projectsList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(
        projectsList,
        &QListWidget::customContextMenuRequested,
        this,
        &MainWindow::showProjectsContextMenu
    );

    board = new BoardView();

    splitter->addWidget(projectsList);
    splitter->addWidget(board);

    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter);
}
