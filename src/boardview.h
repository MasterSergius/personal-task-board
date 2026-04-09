#pragma once

#include <QWidget>

class QHBoxLayout;
class QScrollArea;

class BoardView : public QWidget
{
    Q_OBJECT

public:
    BoardView(QWidget *parent = nullptr);

    void addColumn(const QString &title);

private:
    QWidget *container;
    QHBoxLayout *columnsLayout;
};
