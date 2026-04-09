#pragma once

#include <QMainWindow>

class QListWidget;
class QSplitter;
class BoardView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    void setupUI();
    void showProjectsContextMenu(const QPoint &pos);

    QSplitter *splitter;
    QListWidget *projectsList;
    BoardView *board;
};
