#include "cardwidget.h"

#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QPixmap>
#include <QVBoxLayout>

CardWidget::CardWidget(const QString &text, QWidget *parent)
    : QFrame(parent)
{
    setStyleSheet(R"(
        QFrame {
            border: 1px solid #888;
            border-radius: 6px;
            background: #f5f5f5;
        }
    )");

    auto *layout = new QVBoxLayout(this);

    auto *label = new QLabel(text);
    label->setWordWrap(true);

    layout->addWidget(label);
}

void CardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Record the position of the click relative to this card's (0,0) point
        dragStartPosition = event->pos();
    }
}

// mouseMoveEvent initiates the drag operation if a certain distance threshold is met
void CardWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Verify the left mouse button is held and we have a start position
    if (!(event->buttons() & Qt::LeftButton) || dragStartPosition.isNull()) {
        return;
    }

    // Check if the current move distance exceeds the standard drag threshold
    if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    // --- Start Dragging Operation ---

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    // Transfer this widget's pointer address. The drop target will use it.
    mimeData->setText(QString::number(reinterpret_cast<quintptr>(this)));
    drag->setMimeData(mimeData);

    // *** The Visual Animation Part ***

    // 1. Take a physical "snapshot" pixmap of this entire CardWidget
    QPixmap pixmap = this->grab();

    // 2. Set this pixmap as the visual drag image
    drag->setPixmap(pixmap);

    // 3. Critically: Set the "hot spot". This synchronizes the clicked point
    // of the snapshot exactly under the cursor, creating the perfect follow effect.
    drag->setHotSpot(dragStartPosition);

    // Make the original card invisible in its old column while dragging
    this->hide();

    // Execute the drag. THIS IS A BLOCKING CALL until the drag finishes.
    Qt::DropAction result = drag->exec(Qt::MoveAction);

    // Handle the result: If it was not moved, or drag canceled, reappear.
    // If successful, ColumnWidget::dropEvent() has already added it to its layout.
    if (result != Qt::MoveAction) {
        // Drag failed or was cancelled
        this->show();
    } else {
        // Drag succeeded. The widget was reparented via ColumnWidget::dropEvent().
        // We MUST call show() because it was hidden before drag->exec().
        this->show();
    }

    // Reset the start position for the next potential drag
    dragStartPosition = QPoint();
}
