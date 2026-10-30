#include <QDir>
#include <QFile>
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

    // ── Project names ─────────────────────────────────────────────────────────

    void isValidProjectName_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("valid");

        QTest::newRow("plain")        << QString("Work")           << true;
        QTest::newRow("spaces")       << QString("Side project")   << true;
        QTest::newRow("dots")         << QString("v1.2 plan")      << true;
        QTest::newRow("unicode")      << QString("Проект ✓")       << true;
        QTest::newRow("empty")        << QString("")               << false;
        QTest::newRow("padded")       << QString(" Work ")         << false;
        QTest::newRow("slash")        << QString("a/b")            << false;
        QTest::newRow("traversal")    << QString("../escape")      << false;
        QTest::newRow("backslash")    << QString("a\\b")           << false;
        QTest::newRow("hidden")       << QString(".hidden")        << false;
        QTest::newRow("dotdot")       << QString("..")             << false;
        QTest::newRow("colon")        << QString("a:b")            << false;
        QTest::newRow("wildcard")     << QString("a*")             << false;
        QTest::newRow("newline")      << QString("a\nb")           << false;
    }

    void isValidProjectName()
    {
        QFETCH(QString, name);
        QFETCH(bool, valid);
        QCOMPARE(AppState::isValidProjectName(name), valid);
    }

    void addProject_invalidName_isRejectedAndNothingWritten()
    {
        auto [s, dir] = makeState();

        QCOMPARE(s.addProject("../escape"), -1);
        QCOMPARE(s.addProject(".hidden"), -1);
        QCOMPARE(s.projectCount(), 0);
        QVERIFY(!QFile::exists(dir.path() + "/../escape.md"));
        QVERIFY(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).isEmpty());
    }

    void addProject_duplicateDifferingOnlyInCase_isRejected()
    {
        auto [s, dir] = makeState();

        s.addProject("Work");
        QCOMPARE(s.addProject("work"), -1);
        QCOMPARE(s.projectCount(), 1);
    }

    void addProject_nameWithDots_survivesReload()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        {
            AppState s(dir.path());
            s.load();
            s.addProject("v1.2 plan");
        }
        AppState s2(dir.path());
        s2.load();
        QCOMPARE(s2.projectNames(), QStringList({"v1.2 plan"}));
    }

    void renameProject_invalidName_returnsFalse()
    {
        auto [s, dir] = makeState();

        s.addProject("A");
        QVERIFY(!s.renameProject(0, "x/y"));
        QCOMPARE(s.project(0).name, QString("A"));
    }

    void renameProject_caseOnlyChange_isAllowed()
    {
        auto [s, dir] = makeState();

        s.addProject("work");
        QVERIFY(s.renameProject(0, "Work"));
        QCOMPARE(s.project(0).name, QString("Work"));

        AppState s2(dir.path());
        s2.load();
        QCOMPARE(s2.projectNames(), QStringList({"Work"}));
    }

    void renameProject_targetFileExistsOnDisk_failsWithoutChanges()
    {
        auto [s, dir] = makeState();

        s.addProject("A");
        s.addColumn(0, "Mine");

        // A board created outside the app after load()
        QFile f(dir.path() + "/B.md");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("## Theirs\n");
        f.close();

        QVERIFY(!s.renameProject(0, "B"));
        QCOMPARE(s.project(0).name, QString("A"));

        AppState s2(dir.path());
        s2.load();
        QCOMPARE(s2.projectNames(), QStringList({"A", "B"}));
        QCOMPARE(s2.project(1).columns[0].name, QString("Theirs"));
    }

    void renameProject_missingFile_isRecreated()
    {
        auto [s, dir] = makeState();

        s.addProject("A");
        s.addColumn(0, "Col");
        QVERIFY(QFile::remove(dir.path() + "/A.md"));

        QVERIFY(s.renameProject(0, "B"));

        AppState s2(dir.path());
        s2.load();
        QCOMPARE(s2.projectNames(), QStringList({"B"}));
        QCOMPARE(s2.project(0).columns[0].name, QString("Col"));
    }

    // ── Robustness ────────────────────────────────────────────────────────────

    void outOfRangeIndices_areIgnored()
    {
        auto [s, dir] = makeState();

        s.addProject("P");
        s.addColumn(0, "A");
        s.addColumn(0, "B");
        s.addTask(0, 0, "T");

        // None of these may crash or change anything
        s.removeProject(5);
        s.removeProject(-1);
        QVERIFY(!s.renameProject(3, "X"));
        s.addColumn(2, "X");
        s.removeColumn(0, 7);
        s.renameColumn(0, -1, "X");
        s.moveColumn(0, 0, 9);
        s.moveColumn(0, -1, 0);
        s.addTask(0, 4, "X");
        s.removeTask(0, 0, 3);
        s.removeTask(0, 3, 0);
        s.moveTask(0, 0, 9, 1);
        s.moveTask(0, 0, 0, 9);
        s.moveTask(0, 9, 0, 1);

        QCOMPARE(s.projectCount(), 1);
        QCOMPARE(s.project(0).columns.size(), 2);
        QCOMPARE(s.project(0).columns[0].name, QString("A"));
        QCOMPARE(s.project(0).columns[0].tasks.size(), 1);
        QCOMPARE(s.project(0).columns[1].tasks.size(), 0);
    }

    void moveTask_preservesDoneState()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile f(dir.path() + "/P.md");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("## Todo\n\n- [x] Finished\n\n## Done\n");
        f.close();

        {
            AppState s(dir.path());
            s.load();
            s.moveTask(0, 0, 0, 1);
            QCOMPARE(s.project(0).columns[1].tasks[0].done, true);
        }

        AppState s2(dir.path());
        s2.load();
        QCOMPARE(s2.project(0).columns[1].tasks[0].title, QString("Finished"));
        QCOMPARE(s2.project(0).columns[1].tasks[0].done,  true);
    }

    void mutations_keepObsidianFrontMatter()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile f(dir.path() + "/Obs.md");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("---\nkanban-plugin: board\n---\n\n## Todo\n\n- [ ] A\n\n%% kanban:settings\n%%\n");
        f.close();

        AppState s(dir.path());
        s.load();
        s.addTask(0, 0, "B");

        QVERIFY(f.open(QIODevice::ReadOnly));
        const QString saved = QString::fromUtf8(f.readAll());
        QVERIFY(saved.startsWith("---\nkanban-plugin: board\n---\n"));
        QVERIFY(saved.contains("- [ ] B\n"));
        QVERIFY(saved.trimmed().endsWith("%% kanban:settings\n%%"));
    }
    // ── Priority / color ──────────────────────────────────────────────────────

    void setTaskPriorityAndColor_persist()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        {
            AppState s(dir.path());
            s.load();
            s.addProject("P");
            s.addColumn(0, "Col");
            s.addTask(0, 0, "T");
            s.setTaskPriority(0, 0, 0, Priority::Urgent);
            QVERIFY(s.setTaskColor(0, 0, 0, "#3E63DD"));
            QCOMPARE(s.project(0).columns[0].tasks[0].color, QString("#3e63dd"));
        }

        AppState s2(dir.path());
        s2.load();
        const Task &t = s2.project(0).columns[0].tasks[0];
        QCOMPARE(t.title,    QString("T"));
        QCOMPARE(t.priority, Priority::Urgent);
        QCOMPARE(t.color,    QString("#3e63dd"));
    }

    void setTaskColor_invalid_isRejected_emptyResets()
    {
        auto [s, dir] = makeState();
        s.addProject("P");
        s.addColumn(0, "Col");
        s.addTask(0, 0, "T");

        QVERIFY(s.setTaskColor(0, 0, 0, "#30a46c"));
        QVERIFY(!s.setTaskColor(0, 0, 0, "green"));
        QCOMPARE(s.project(0).columns[0].tasks[0].color, QString("#30a46c"));

        QVERIFY(s.setTaskColor(0, 0, 0, QString()));
        QVERIFY(s.project(0).columns[0].tasks[0].color.isEmpty());
    }

    void setTaskPriorityAndColor_outOfRange_areIgnored()
    {
        auto [s, dir] = makeState();
        s.addProject("P");
        s.addColumn(0, "Col");
        s.addTask(0, 0, "T");

        s.setTaskPriority(0, 0, 5, Priority::High);
        s.setTaskPriority(0, 3, 0, Priority::High);
        s.setTaskPriority(2, 0, 0, Priority::High);
        QVERIFY(!s.setTaskColor(0, 0, -1, "#ffffff"));
        QVERIFY(!s.setTaskColor(1, 0, 0, "#ffffff"));

        QCOMPARE(s.project(0).columns[0].tasks[0].priority, Priority::None);
        QVERIFY(s.project(0).columns[0].tasks[0].color.isEmpty());
    }

    void moveTask_keepsPriorityAndColor()
    {
        auto [s, dir] = makeState();
        s.addProject("P");
        s.addColumn(0, "A");
        s.addColumn(0, "B");
        s.addTask(0, 0, "T");
        s.setTaskPriority(0, 0, 0, Priority::Low);
        s.setTaskColor(0, 0, 0, "#d6409f");

        s.moveTask(0, 0, 0, 1);

        const Task &t = s.project(0).columns[1].tasks[0];
        QCOMPARE(t.priority, Priority::Low);
        QCOMPARE(t.color,    QString("#d6409f"));
    }
    // ── Title / description ───────────────────────────────────────────────────

    void setTaskDetails_persists()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        {
            AppState s(dir.path());
            s.load();
            s.addProject("P");
            s.addColumn(0, "Col");
            s.addTask(0, 0, "Old");
            s.setTaskPriority(0, 0, 0, Priority::High);
            QVERIFY(s.setTaskDetails(0, 0, 0, "  New title ", "Some **notes**\n\n- [ ] step"));
        }

        AppState s2(dir.path());
        s2.load();
        const Task &t = s2.project(0).columns[0].tasks[0];
        QCOMPARE(t.title,       QString("New title"));
        QCOMPARE(t.description, QString("Some **notes**\n\n- [ ] step"));
        QCOMPARE(t.priority,    Priority::High);
    }

    void setTaskDetails_normalizesInput()
    {
        auto [s, dir] = makeState();
        s.addProject("P");
        s.addColumn(0, "Col");
        s.addTask(0, 0, "T");

        QVERIFY(s.setTaskDetails(0, 0, 0, "two\nlines", "\n\r\nline 1\r\nline 2  \n\n"));
        const Task &t = s.project(0).columns[0].tasks[0];
        QCOMPARE(t.title,       QString("two lines"));
        QCOMPARE(t.description, QString("line 1\nline 2"));

        QVERIFY(s.setTaskDetails(0, 0, 0, "T", "   "));
        QVERIFY(s.project(0).columns[0].tasks[0].description.isEmpty());
    }

    void setTaskDetails_invalid_isRejected()
    {
        auto [s, dir] = makeState();
        s.addProject("P");
        s.addColumn(0, "Col");
        s.addTask(0, 0, "T");

        QVERIFY(!s.setTaskDetails(0, 0, 0, "  \n ", "desc"));
        QVERIFY(!s.setTaskDetails(0, 0, 1, "X", "desc"));
        QVERIFY(!s.setTaskDetails(0, 1, 0, "X", "desc"));
        QVERIFY(!s.setTaskDetails(1, 0, 0, "X", "desc"));

        const Task &t = s.project(0).columns[0].tasks[0];
        QCOMPARE(t.title, QString("T"));
        QVERIFY(t.description.isEmpty());
    }

    void moveTask_keepsDescription()
    {
        auto [s, dir] = makeState();
        s.addProject("P");
        s.addColumn(0, "A");
        s.addColumn(0, "B");
        s.addTask(0, 0, "T");
        s.setTaskDetails(0, 0, 0, "T", "notes");

        s.moveTask(0, 0, 0, 1);
        QCOMPARE(s.project(0).columns[1].tasks[0].description, QString("notes"));
    }
};

QTEST_GUILESS_MAIN(TestAppState)
#include "test_appstate.moc"
