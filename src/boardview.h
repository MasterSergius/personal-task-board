#pragma once

#include <QWidget>
#include "model/appstate.h"

class QHBoxLayout;
class QPushButton;

/**
 * The main board area: a horizontally-scrollable row of ColumnWidgets.
 *
 * BoardView owns the column layout and is responsible for rebuilding it
 * (via refresh()) whenever the underlying model changes. It connects to
 * ColumnWidget signals and routes all state mutations through AppState.
 */
class BoardView : public QWidget
{
    Q_OBJECT

public:
    explicit BoardView(AppState &state, QWidget *parent = nullptr);

    /** Switch to the given project and rebuild the board. */
    void loadProject(int projectIdx);

    /** Clear the board (no project selected). */
    void clear();

    /** Rebuild all column widgets from the current model state. */
    void refresh();

private slots:
    void onAddColumnClicked();

private:
    AppState &m_state;
    int m_projectIdx = -1;

    QHBoxLayout *m_columnsLayout = nullptr;
    QPushButton *m_addColumnBtn  = nullptr;
};
