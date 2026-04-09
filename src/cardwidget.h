#pragma once

#include <QFrame>
#include <QPoint>

class QLabel;
class QMouseEvent; 

class CardWidget : public QFrame
{
    Q_OBJECT

public:
    CardWidget(const QString &text, QWidget *parent = nullptr);

protected:
    // We must declare the event we are overriding from QFrame/QWidget
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint dragStartPosition;
};

