#include "cardwidget.h"

#include <QActionGroup>
#include <QApplication>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QDrag>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

// ─── Priority / color presentation ────────────────────────────────────────────

const QList<QPair<QString, QString>> &CardWidget::framePalette()
{
    static const QList<QPair<QString, QString>> palette = {
        {"Red",    "#e5484d"},
        {"Orange", "#f76b15"},
        {"Yellow", "#ffc53d"},
        {"Green",  "#30a46c"},
        {"Teal",   "#12a594"},
        {"Blue",   "#3e63dd"},
        {"Purple", "#8e4ec6"},
        {"Pink",   "#d6409f"},
        {"Gray",   "#8b8d98"},
    };
    return palette;
}

QString CardWidget::priorityLabel(Priority priority)
{
    switch (priority) {
    case Priority::Low:    return "LOW";
    case Priority::Medium: return "MEDIUM";
    case Priority::High:   return "HIGH";
    case Priority::Urgent: return "URGENT";
    case Priority::None:   break;
    }
    return {};
}

QString CardWidget::priorityColor(Priority priority)
{
    switch (priority) {
    case Priority::Low:    return "#8b8d98";
    case Priority::Medium: return "#3e63dd";
    case Priority::High:   return "#f76b15";
    case Priority::Urgent: return "#e5484d";
    case Priority::None:   break;
    }
    return {};
}

// Small filled circle used as a menu icon.
static QIcon swatchIcon(const QString &color)
{
    QPixmap pm(12, 12);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(color));
    p.drawEllipse(pm.rect().adjusted(1, 1, -1, -1));
    return QIcon(pm);
}

// ─── Construction ─────────────────────────────────────────────────────────────

CardWidget::CardWidget(const Task &task, int projectIdx, int colIdx, int taskIdx,
                       const Theme &theme, QWidget *parent)
    : QFrame(parent)
    , m_projectIdx(projectIdx)
    , m_colIdx(colIdx)
    , m_taskIdx(taskIdx)
    , m_priority(task.priority)
    , m_color(task.color)
    , m_theme(theme)
{
    QString style = theme.cardStyle;
    if (!m_color.isEmpty()) {
        // Appended rules override the theme's border; the card's child labels
        // set their own "border: none" so the frame isn't repeated on them.
        style += QString("QFrame { border: 2px solid %1; }"
                         "QFrame:hover { border-color: %1; }").arg(m_color);
    }
    setStyleSheet(style);
    setCursor(Qt::OpenHandCursor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);

    if (m_priority != Priority::None) {
        m_badge = new QLabel(priorityLabel(m_priority));
        m_badge->setObjectName("priorityBadge");
        m_badge->setStyleSheet(QString(
            "border: none; border-radius: 4px; padding: 1px 6px;"
            "background: %1; color: #ffffff; font-size: 10px; font-weight: bold;")
                .arg(priorityColor(m_priority)));

        auto *badgeRow = new QHBoxLayout();
        badgeRow->setContentsMargins(0, 0, 0, 0);
        badgeRow->addWidget(m_badge);
        badgeRow->addStretch();
        layout->addLayout(badgeRow);
    }

    m_label = new QLabel(task.title);
    m_label->setWordWrap(true);
    m_label->setStyleSheet(theme.cardLabelStyle);
    layout->addWidget(m_label);
}

// ─── Drag ─────────────────────────────────────────────────────────────────────

void CardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
}

void CardWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_dragStartPos.isNull())
        return;

    if ((event->pos() - m_dragStartPos).manhattanLength() < QApplication::startDragDistance())
        return;

    const QPoint hotSpot = m_dragStartPos;
    m_dragStartPos = QPoint();
    m_dropColIdx   = -1;

    auto *drag     = new QDrag(this);
    auto *mimeData = new QMimeData();

    // Private MIME type: the drop target only accepts drags from this app.
    // Indices travel via the source widget, not the payload.
    mimeData->setData(MimeType, QByteArray());
    drag->setMimeData(mimeData);

    // Snapshot the card as the drag cursor image
    drag->setPixmap(grab());
    drag->setHotSpot(hotSpot);

    // Hide original while dragging
    hide();

    const Qt::DropAction result = drag->exec(Qt::MoveAction);

    // The model update happens here, after exec() has returned, rather than in
    // the target's dropEvent(): rebuilding the board deletes this card, and
    // doing that while still inside the drag's nested event loop would leave
    // exec() running on a destroyed QDrag.
    if (result == Qt::MoveAction && m_dropColIdx >= 0 && m_dropColIdx != m_colIdx) {
        emit moveRequested(m_colIdx, m_taskIdx, m_dropColIdx);   // board rebuild follows
        return;
    }

    show();
}

// ─── Context menu ─────────────────────────────────────────────────────────────

void CardWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.setStyleSheet(m_theme.menuStyle);

    // Actions are only mapped to values here and handled after exec() returns:
    // the handlers rebuild the board (deleting this card), which must not
    // happen while the menu's nested event loop is still running.
    QHash<QAction *, Priority> priorityActions;
    QHash<QAction *, QString>  colorActions;   // empty value = theme default

    // ── Priority submenu ──────────────────────────────────────────────────────
    QMenu *priorityMenu = menu.addMenu("Priority");
    auto *priorityGroup = new QActionGroup(priorityMenu);
    for (Priority p : {Priority::None, Priority::Urgent, Priority::High,
                       Priority::Medium, Priority::Low}) {
        const QString label = (p == Priority::None)
            ? QString("None")
            : priorityLabel(p).left(1) + priorityLabel(p).mid(1).toLower();
        QAction *a = priorityMenu->addAction(label);
        if (p != Priority::None)
            a->setIcon(swatchIcon(priorityColor(p)));
        a->setCheckable(true);
        a->setChecked(p == m_priority);
        a->setActionGroup(priorityGroup);
        priorityActions.insert(a, p);
    }

    // ── Frame color submenu ───────────────────────────────────────────────────
    QMenu *colorMenu = menu.addMenu("Frame Color");
    auto *colorGroup = new QActionGroup(colorMenu);

    QAction *defaultColor = colorMenu->addAction("Default");
    defaultColor->setCheckable(true);
    defaultColor->setChecked(m_color.isEmpty());
    defaultColor->setActionGroup(colorGroup);
    colorActions.insert(defaultColor, QString());
    colorMenu->addSeparator();

    bool isPreset = m_color.isEmpty();
    for (const auto &[name, hex] : framePalette()) {
        QAction *a = colorMenu->addAction(swatchIcon(hex), name);
        a->setCheckable(true);
        a->setChecked(hex == m_color);
        a->setActionGroup(colorGroup);
        colorActions.insert(a, hex);
        isPreset = isPreset || hex == m_color;
    }
    colorMenu->addSeparator();

    QAction *customColor = colorMenu->addAction("Custom…");
    if (!isPreset) {
        customColor->setIcon(swatchIcon(m_color));
        customColor->setCheckable(true);
        customColor->setChecked(true);
        customColor->setActionGroup(colorGroup);
    }

    menu.addSeparator();
    QAction *deleteAction = menu.addAction("Delete Task");

    QAction *selected = menu.exec(event->globalPos());
    if (!selected)
        return;

    if (priorityActions.contains(selected)) {
        const Priority p = priorityActions.value(selected);
        if (p != m_priority)
            emit priorityChangeRequested(m_colIdx, m_taskIdx, p);
        return;
    }

    if (colorActions.contains(selected)) {
        const QString c = colorActions.value(selected);
        if (c != m_color)
            emit colorChangeRequested(m_colIdx, m_taskIdx, c);
        return;
    }

    if (selected == customColor) {
        const QColor initial = m_color.isEmpty() ? QColor(framePalette().first().second)
                                                 : QColor(m_color);
        const QColor c = QColorDialog::getColor(initial, this, "Frame Color");
        if (c.isValid() && c.name() != m_color)
            emit colorChangeRequested(m_colIdx, m_taskIdx, c.name());
        return;
    }

    if (selected == deleteAction) {
        auto reply = QMessageBox::question(
            this, "Delete Task",
            QString("Delete task \"%1\"?").arg(m_label->text()),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
            emit deleteRequested(m_colIdx, m_taskIdx);
    }
}
