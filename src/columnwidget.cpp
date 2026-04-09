#include "columnwidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "cardwidget.h"

ColumnWidget::ColumnWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *label = new QLabel(title);
    label->setStyleSheet("font-weight: bold;");
    layout->addWidget(label);

    auto *addBtn = new QPushButton("+");
    layout->addWidget(addBtn);

    cardsLayout = new QVBoxLayout();
    layout->addLayout(cardsLayout);

    layout->addStretch();

    // CRITICAL: Tell Qt that this widget can accept drop events
    setAcceptDrops(true);

    // test cards
    addCard("Task 1");
    addCard("Task 2");

    connect(addBtn, &QPushButton::clicked, this, [this]() {
        addCard("New Task");
    });
}

void ColumnWidget::addCard(const QString &text)
{
    auto *card = new CardWidget(text);
    cardsLayout->addWidget(card);
}

// Event triggered when a dragged object enters the widget area
void ColumnWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept the event if it contains text data (our widget pointer)
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

// Event triggered when the object is dropped
void ColumnWidget::dropEvent(QDropEvent *event)
{
    // Retrieve the pointer address from the text
    bool ok;
    quintptr ptrAddress = event->mimeData()->text().toULongLong(&ok);

    if (ok) {
        // Convert the address back to a CardWidget pointer
        CardWidget *card = reinterpret_cast<CardWidget*>(ptrAddress);

        if (card) {
            // Remove the card from its previous layout and add it to this column's layout
            cardsLayout->addWidget(card);
            event->acceptProposedAction();
        }
    }
}
