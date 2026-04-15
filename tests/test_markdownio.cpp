#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "model/markdownio.h"

class TestMarkdownIO : public QObject
{
    Q_OBJECT

private slots:
    // ── Round-trip ────────────────────────────────────────────────────────────

    void saveAndLoad_preservesAllData()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Project p;
        p.name    = "My Board";
        p.columns = {
            Column{"To Do", {Task{"First task"}, Task{"Second task"}}},
            Column{"Doing", {Task{"In progress"}}},
            Column{"Done",  {}},
        };

        MarkdownIO::save(dir.path(), p);

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects.size(), 1);

        const Project &got = projects[0];
        QCOMPARE(got.name,                           QString("My Board"));
        QCOMPARE(got.columns.size(),                 3);

        QCOMPARE(got.columns[0].name,                QString("To Do"));
        QCOMPARE(got.columns[0].tasks.size(),        2);
        QCOMPARE(got.columns[0].tasks[0].title,      QString("First task"));
        QCOMPARE(got.columns[0].tasks[1].title,      QString("Second task"));

        QCOMPARE(got.columns[1].name,                QString("Doing"));
        QCOMPARE(got.columns[1].tasks.size(),        1);
        QCOMPARE(got.columns[1].tasks[0].title,      QString("In progress"));

        QCOMPARE(got.columns[2].name,                QString("Done"));
        QCOMPARE(got.columns[2].tasks.size(),        0);
    }

    void save_overwrites_existingFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Project p{"Board", {Column{"Col", {Task{"Old"}}}}};
        MarkdownIO::save(dir.path(), p);

        p.columns[0].tasks[0].title = "New";
        MarkdownIO::save(dir.path(), p);

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects[0].columns[0].tasks[0].title, QString("New"));
    }

    // ── Ordering ──────────────────────────────────────────────────────────────

    void loadAll_returnsProjectsInAlphabeticalOrder()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        MarkdownIO::save(dir.path(), Project{"Zeta",  {}});
        MarkdownIO::save(dir.path(), Project{"Alpha", {}});
        MarkdownIO::save(dir.path(), Project{"Mu",    {}});

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects.size(), 3);
        QCOMPARE(projects[0].name, QString("Alpha"));
        QCOMPARE(projects[1].name, QString("Mu"));
        QCOMPARE(projects[2].name, QString("Zeta"));
    }

    // ── Edge cases ────────────────────────────────────────────────────────────

    void loadAll_emptyDirectory_returnsEmpty()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(MarkdownIO::loadAll(dir.path()).isEmpty());
    }

    void loadAll_nonexistentDirectory_returnsEmpty()
    {
        QVERIFY(MarkdownIO::loadAll("/tmp/ptb-no-such-dir-xyz987").isEmpty());
    }

    // ── Remove / rename ───────────────────────────────────────────────────────

    void remove_deletesFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        MarkdownIO::save(dir.path(), Project{"Gone", {}});
        QCOMPARE(MarkdownIO::loadAll(dir.path()).size(), 1);

        MarkdownIO::remove(dir.path(), "Gone");
        QVERIFY(MarkdownIO::loadAll(dir.path()).isEmpty());
    }

    void rename_movesFileAndPreservesContent()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Project p{"OldName", {Column{"Col", {Task{"T1"}, Task{"T2"}}}}};
        MarkdownIO::save(dir.path(), p);

        MarkdownIO::rename(dir.path(), "OldName", "NewName");

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects.size(), 1);
        QCOMPARE(projects[0].name,                      QString("NewName"));
        QCOMPARE(projects[0].columns[0].tasks[0].title, QString("T1"));
        QCOMPARE(projects[0].columns[0].tasks[1].title, QString("T2"));
    }

    // ── Format ────────────────────────────────────────────────────────────────

    void parse_checkedTasks_areLoadedLikeUnchecked()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // Write a raw file with both checked and unchecked tasks
        QFile f(dir.path() + "/Mix.md");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write("## Col\n\n- [ ] Pending\n- [x] Completed\n");
        f.close();

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects.size(), 1);
        QCOMPARE(projects[0].columns[0].tasks.size(), 2);
        QCOMPARE(projects[0].columns[0].tasks[0].title, QString("Pending"));
        QCOMPARE(projects[0].columns[0].tasks[1].title, QString("Completed"));
    }

    void parse_linesWithoutPrefix_areIgnored()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile f(dir.path() + "/Misc.md");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write("# Ignored heading\n\nSome prose.\n\n## Real Column\n\n- [ ] Task\n");
        f.close();

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects[0].columns.size(), 1);
        QCOMPARE(projects[0].columns[0].name,            QString("Real Column"));
        QCOMPARE(projects[0].columns[0].tasks.size(),    1);
        QCOMPARE(projects[0].columns[0].tasks[0].title,  QString("Task"));
    }
};

QTEST_GUILESS_MAIN(TestMarkdownIO)
#include "test_markdownio.moc"
