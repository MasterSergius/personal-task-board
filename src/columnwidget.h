#pragma once

#include <QWidget>

class QVBoxLayout;
class QDragEnterEvent;
class QDropEvent;

class ColumnWidget : public QWidget
{
    Q_OBJECT

public:
    ColumnWidget(const QString &title, QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QVBoxLayout *cardsLayout;

    void addCard(const QString &text);
};
