#pragma once

#include <QMainWindow>
#include "model/appstate.h"
#include "theme.h"

class QLabel;
class QListWidget;
class QPushButton;
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
    void toggleTheme();

private:
    void setupUI();
    void refreshProjectList(int selectRow = -1);
    void applyTheme();

    void addProjectDialog();
    void renameProjectDialog();
    void deleteCurrentProject();

    /** Returns ~/.ptb/projects where board .md files are stored. */
    static QString boardsDirectory();

    AppState m_state;
    Theme    m_theme = Theme::light();

    QSplitter   *m_splitter     = nullptr;
    QWidget     *m_sidePanel    = nullptr;
    QLabel      *m_sideTitle    = nullptr;
    QPushButton *m_toggleBtn    = nullptr;
    QListWidget *m_projectList  = nullptr;
    BoardView   *m_board        = nullptr;
};
