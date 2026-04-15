#pragma once

#include <QMainWindow>
#include "model/appstate.h"

class QListWidget;
class QSplitter;
class BoardView;

/**
 * Application main window.
 *
 * Left panel: project list (QListWidget) with right-click context menu
 *             for add / rename / delete.
 * Right panel: BoardView for the currently selected project.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onProjectSelectionChanged(int row);
    void showProjectContextMenu(const QPoint &pos);

private:
    void setupUI();
    void refreshProjectList(int selectRow = -1);

    void addProjectDialog();
    void renameProjectDialog();
    void deleteCurrentProject();

    /** Returns ~/.ptb/projects where board .md files are stored. */
    static QString boardsDirectory();

    AppState m_state;

    QSplitter   *m_splitter     = nullptr;
    QListWidget *m_projectList  = nullptr;
    BoardView   *m_board        = nullptr;
};
