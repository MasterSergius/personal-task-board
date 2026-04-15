#include <QTemporaryDir>
#include <QTest>

#include "model/appstate.h"

// Helper: create a fresh AppState backed by a temp directory and call load().
// Usage:  auto [s, dir] = makeState();  (dir keeps the temp dir alive)
static std::pair<AppState, QTemporaryDir> makeState()
{
    QTemporaryDir dir;
    Q_ASSERT(dir.isValid());
    AppState s(dir.path());
    s.load();
    return {std::move(s), std::move(dir)};
}

class TestAppState : public QObject
{
    Q_OBJECT

private slots:
    // ── Projects ──────────────────────────────────────────────────────────────

    void addProject_incrementsCount()
    {
        auto [s, dir] = makeState();

        QCOMPARE(s.projectCount(), 0);
        int idx = s.addProject("Alpha");
        QCOMPARE(idx, 0);
        QCOMPARE(s.projectCount(), 1);
        QCOMPARE(s.project(0).name, QString("Alpha"));
    }

    void addProject_duplicateName_returnsMinusOne()
    {
        auto [s, dir] = makeState();

        s.addProject("Alpha");
        int dup = s.addProject("Alpha");
        QCOMPARE(dup, -1);
        QCOMPARE(s.projectCount(), 1);
    }

    void removeProject_reducesCount()
    {
        auto [s, dir] = makeState();

        s.addProject("A");
        s.addProject("B");
        s.removeProject(0);

        QCOMPARE(s.projectCount(), 1);
        QCOMPARE(s.project(0).name, QString("B"));
    }

    void renameProject_updatesName()
    {
        auto [s, dir] = makeState();

        s.addProject("Old");
        QVERIFY(s.renameProject(0, "New"));
        QCOMPARE(s.project(0).name, QString("New"));
    }

    void renameProject_duplicateName_returnsFalse()
    {
        auto [s, dir] = makeState();

        s.addProject("A");
        s.addProject("B");
        QVERIFY(!s.renameProject(0, "B"));
        QCOMPARE(s.project(0).name, QString("A")); // unchanged
    }

    void projectNames_returnsAllNames()
    {
        auto [s, dir] = makeState();

        s.addProject("X");
        s.addProject("Y");
        QCOMPARE(s.projectNames(), QStringList({"X", "Y"}));
    }

    // ── Columns ───────────────────────────────────────────────────────────────

    void addColumn_appendsToProject()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "To Do");
        s.addColumn(0, "Done");

        QCOMPARE(s.project(0).columns.size(), 2);
        QCOMPARE(s.project(0).columns[0].name, QString("To Do"));
        QCOMPARE(s.project(0).columns[1].name, QString("Done"));
    }

    void removeColumn_removesCorrectColumn()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "A");
        s.addColumn(0, "B");
        s.addColumn(0, "C");
        s.removeColumn(0, 1); // remove "B"

        QCOMPARE(s.project(0).columns.size(), 2);
        QCOMPARE(s.project(0).columns[0].name, QString("A"));
        QCOMPARE(s.project(0).columns[1].name, QString("C"));
    }

    void renameColumn_updatesName()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "Old");
        s.renameColumn(0, 0, "New");

        QCOMPARE(s.project(0).columns[0].name, QString("New"));
    }

    void moveColumn_reordersCorrectly()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "A");
        s.addColumn(0, "B");
        s.addColumn(0, "C");

        // Move A (index 0) to the end (index 2) → [B, C, A]
        s.moveColumn(0, 0, 2);
        QCOMPARE(s.project(0).columns[0].name, QString("B"));
        QCOMPARE(s.project(0).columns[1].name, QString("C"));
        QCOMPARE(s.project(0).columns[2].name, QString("A"));

        // Move A back to front → [A, B, C]
        s.moveColumn(0, 2, 0);
        QCOMPARE(s.project(0).columns[0].name, QString("A"));
        QCOMPARE(s.project(0).columns[1].name, QString("B"));
        QCOMPARE(s.project(0).columns[2].name, QString("C"));
    }

    void moveColumnLeft_atLeftBoundary_isNoOp()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "A");
        s.addColumn(0, "B");
        s.moveColumnLeft(0, 0); // already leftmost

        QCOMPARE(s.project(0).columns[0].name, QString("A"));
        QCOMPARE(s.project(0).columns[1].name, QString("B"));
    }

    void moveColumnRight_atRightBoundary_isNoOp()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "A");
        s.addColumn(0, "B");
        s.moveColumnRight(0, 1); // already rightmost

        QCOMPARE(s.project(0).columns[0].name, QString("A"));
        QCOMPARE(s.project(0).columns[1].name, QString("B"));
    }

    // ── Tasks ─────────────────────────────────────────────────────────────────

    void addTask_appendsToColumn()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "Col");
        s.addTask(0, 0, "Task A");
        s.addTask(0, 0, "Task B");

        const auto &tasks = s.project(0).columns[0].tasks;
        QCOMPARE(tasks.size(), 2);
        QCOMPARE(tasks[0].title, QString("Task A"));
        QCOMPARE(tasks[1].title, QString("Task B"));
    }

    void removeTask_removesCorrectTask()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "Col");
        s.addTask(0, 0, "A");
        s.addTask(0, 0, "B");
        s.addTask(0, 0, "C");
        s.removeTask(0, 0, 1); // remove "B"

        const auto &tasks = s.project(0).columns[0].tasks;
        QCOMPARE(tasks.size(), 2);
        QCOMPARE(tasks[0].title, QString("A"));
        QCOMPARE(tasks[1].title, QString("C"));
    }

    void moveTask_movesAcrossColumns()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "Todo");
        s.addColumn(0, "Done");
        s.addTask(0, 0, "Task X");

        s.moveTask(0, /*fromCol*/0, /*taskIdx*/0, /*toCol*/1);

        QVERIFY(s.project(0).columns[0].tasks.isEmpty());
        QCOMPARE(s.project(0).columns[1].tasks.size(), 1);
        QCOMPARE(s.project(0).columns[1].tasks[0].title, QString("Task X"));
    }

    // ── Persistence ───────────────────────────────────────────────────────────

    void persistence_dataSurvivesReload()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // Write state with first AppState instance
        {
            AppState s(dir.path());
            s.load();
            s.addProject("Board");
            s.addColumn(0, "Todo");
            s.addTask(0, 0, "Survive");
        }

        // Read it back with a fresh instance
        AppState s2(dir.path());
        s2.load();

        QCOMPARE(s2.projectCount(), 1);
        QCOMPARE(s2.project(0).name,                      QString("Board"));
        QCOMPARE(s2.project(0).columns[0].name,           QString("Todo"));
        QCOMPARE(s2.project(0).columns[0].tasks[0].title, QString("Survive"));
    }

    void persistence_renameUpdatesFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        {
            AppState s(dir.path());
            s.load();
            s.addProject("Before");
            s.addColumn(0, "Col");
            s.renameProject(0, "After");
        }

        AppState s2(dir.path());
        s2.load();

        QCOMPARE(s2.projectCount(), 1);
        QCOMPARE(s2.project(0).name, QString("After"));
        QCOMPARE(s2.project(0).columns[0].name, QString("Col"));
    }

    void persistence_removeDeletesFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        {
            AppState s(dir.path());
            s.load();
            s.addProject("Temporary");
            s.removeProject(0);
        }

        AppState s2(dir.path());
        s2.load();
        QCOMPARE(s2.projectCount(), 0);
    }
};

QTEST_GUILESS_MAIN(TestAppState)
#include "test_appstate.moc"
