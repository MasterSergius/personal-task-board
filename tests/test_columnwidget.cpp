#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "cardwidget.h"
#include "columnwidget.h"
#include "model/appstate.h"

// Drag-and-drop acceptance rules for ColumnWidget.
//
// A full drag can't be driven from a test (QDrag::exec() blocks on the
// platform), so these tests deliver the enter/drop events directly.

class TestColumnWidget : public QObject
{
    Q_OBJECT

private:
    static bool sendDragEnter(QWidget *target, QMimeData *mime)
    {
        QDragEnterEvent ev(QPoint(5, 5), Qt::MoveAction, mime, Qt::LeftButton, Qt::NoModifier);
        ev.ignore();
        QCoreApplication::sendEvent(target, &ev);
        return ev.isAccepted();
    }

    static bool sendDrop(QWidget *target, QMimeData *mime)
    {
        QDropEvent ev(QPointF(5, 5), Qt::MoveAction, mime, Qt::LeftButton, Qt::NoModifier);
        ev.ignore();
        QCoreApplication::sendEvent(target, &ev);
        return ev.isAccepted();
    }

private slots:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
        m_state = std::make_unique<AppState>(m_dir->path());
        m_state->load();
        m_state->addProject("P");
        m_state->addColumn(0, "Todo");
        m_state->addColumn(0, "Done");
        m_state->addTask(0, 0, "Task");
    }

    // Text dragged from another application (e.g. "0:0" from an editor) used
    // to be interpreted as a task move and could index out of range.
    void foreignTextDrag_isRejected()
    {
        ColumnWidget col(*m_state, 0, 1, Theme::light());
        QSignalSpy moved(&col, &ColumnWidget::taskMoved);

        QMimeData mime;
        mime.setText("0:0");
        QVERIFY(!sendDragEnter(&col, &mime));
        QVERIFY(!sendDrop(&col, &mime));

        mime.setText("ptb-column:0");
        QVERIFY(!sendDragEnter(&col, &mime));
        QVERIFY(!sendDrop(&col, &mime));

        QCOMPARE(moved.count(), 0);
        QCOMPARE(m_state->project(0).columns[0].tasks.size(), 1);
    }

    // Our MIME type alone is not enough: the drag must originate in this app.
    void privateMimeWithoutSource_isRejected()
    {
        ColumnWidget col(*m_state, 0, 1, Theme::light());

        QMimeData mime;
        mime.setData(CardWidget::MimeType, QByteArray());
        QVERIFY(!sendDragEnter(&col, &mime));
        QVERIFY(!sendDrop(&col, &mime));
    }

    // Dropping does not mutate the model directly; the source emits the move
    // only after QDrag::exec() returns.
    void drop_doesNotTouchModelOrEmitDuringDrag()
    {
        ColumnWidget col(*m_state, 0, 1, Theme::light());
        QSignalSpy moved(&col, &ColumnWidget::taskMoved);

        QMimeData mime;
        mime.setData(CardWidget::MimeType, QByteArray());
        sendDrop(&col, &mime);

        QCOMPARE(moved.count(), 0);
        QCOMPARE(m_state->project(0).columns[0].tasks.size(), 1);
        QCOMPARE(m_state->project(0).columns[1].tasks.size(), 0);
    }

    void cardMoveRequest_isForwardedAsTaskMoved()
    {
        ColumnWidget col(*m_state, 0, 0, Theme::light());
        QSignalSpy moved(&col, &ColumnWidget::taskMoved);

        auto *card = col.findChild<CardWidget *>();
        QVERIFY(card);
        emit card->moveRequested(0, 0, 1);

        QCOMPARE(moved.count(), 1);
        QCOMPARE(moved[0][0].toInt(), 0);
        QCOMPARE(moved[0][1].toInt(), 0);
        QCOMPARE(moved[0][2].toInt(), 1);
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<AppState> m_state;
};

QTEST_MAIN(TestColumnWidget)
#include "test_columnwidget.moc"
