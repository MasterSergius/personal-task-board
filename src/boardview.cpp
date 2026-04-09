#include "boardview.h"

#include <QScrollArea>
#include <QHBoxLayout>
#include <QWidget>

#include "columnwidget.h"

BoardView::BoardView(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QHBoxLayout(this);

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);

    container = new QWidget();
    columnsLayout = new QHBoxLayout(container);

    scroll->setWidget(container);

    mainLayout->addWidget(scroll);

    // test data
    addColumn("To Do");
    addColumn("In Progress");
    addColumn("Done");
}

void BoardView::addColumn(const QString &title)
{
    auto *column = new ColumnWidget(title);
    columnsLayout->addWidget(column);
}
