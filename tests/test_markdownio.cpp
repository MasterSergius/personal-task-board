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

    void parse_checkedTasks_keepDoneState()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // Write a raw file with both checked and unchecked tasks
        QFile f(dir.path() + "/Mix.md");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write("## Col\n\n- [ ] Pending\n- [x] Completed\n- [X] Upper\n");
        f.close();

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects.size(), 1);
        const auto &tasks = projects[0].columns[0].tasks;
        QCOMPARE(tasks.size(), 3);
        QCOMPARE(tasks[0].title, QString("Pending"));
        QCOMPARE(tasks[0].done,  false);
        QCOMPARE(tasks[1].title, QString("Completed"));
        QCOMPARE(tasks[1].done,  true);
        QCOMPARE(tasks[2].title, QString("Upper"));
        QCOMPARE(tasks[2].done,  true);
    }

    void save_preservesDoneState()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        MarkdownIO::save(dir.path(), Project{"B", {Column{"C", {Task{"open"}, Task{"closed", true}}}}});

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects[0].columns[0].tasks[0].done, false);
        QCOMPARE(projects[0].columns[0].tasks[1].done, true);
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

    void parse_taskLinesBeforeFirstColumn_areNotTasks()
    {
        const Project p = MarkdownIO::parse("- [ ] orphan\n\n## Col\n- [ ] real\n", "P");
        QCOMPARE(p.columns.size(), 1);
        QCOMPARE(p.columns[0].tasks.size(), 1);
        QCOMPARE(p.columns[0].tasks[0].title, QString("real"));
    }

    void parse_malformedCheckboxes_areIgnored()
    {
        const Project p = MarkdownIO::parse("## Col\n- [y] no\n- [] no\n- [ ]no\n- [ ] yes\n", "P");
        QCOMPARE(p.columns[0].tasks.size(), 1);
        QCOMPARE(p.columns[0].tasks[0].title, QString("yes"));
    }

    void parse_crlfLineEndings()
    {
        const Project p = MarkdownIO::parse("## To Do\r\n\r\n- [ ] A\r\n- [x] B\r\n", "P");
        QCOMPARE(p.columns.size(), 1);
        QCOMPARE(p.columns[0].name, QString("To Do"));
        QCOMPARE(p.columns[0].tasks.size(), 2);
        QCOMPARE(p.columns[0].tasks[1].title, QString("B"));
        QCOMPARE(p.columns[0].tasks[1].done,  true);
    }

    void loadAll_utf8WithBom()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QFile f(dir.path() + "/Bom.md");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("\xEF\xBB\xBF## Привет\n\n- [ ] Задача\n");
        f.close();

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects[0].columns.size(), 1);
        QCOMPARE(projects[0].columns[0].name,           QString("Привет"));
        QCOMPARE(projects[0].columns[0].tasks[0].title, QString("Задача"));
    }

    // ── Serialisation ─────────────────────────────────────────────────────────

    void serialize_defaultFormatIsUnchanged()
    {
        const Project p{"P", {Column{"To Do", {Task{"A"}, Task{"B", true}}}, Column{"Done", {}}}};
        QCOMPARE(MarkdownIO::serialize(p),
                 QString("## To Do\n\n- [ ] A\n- [x] B\n\n## Done\n\n"));
    }

    void obsidianBoard_roundTripPreservesExtras()
    {
        const QString original =
            "---\n\nkanban-plugin: board\n\n---\n\n"
            "## Todo\n\n- [ ] One\n\n"
            "## Done\n\n- [x] Two\n\n"
            "***\n\n## Archive\n\n- [x] Old\n\n"
            "%% kanban:settings\n```\n{\"kanban-plugin\":\"board\"}\n```\n%%\n";

        const Project p = MarkdownIO::parse(original, "Obs");
        QCOMPARE(p.columns.size(), 2);     // "Archive" belongs to the footer
        QCOMPARE(p.columns[1].name, QString("Done"));
        QVERIFY(p.header.contains("kanban-plugin: board"));
        QVERIFY(p.footer.startsWith("***"));
        QVERIFY(p.footer.contains("%% kanban:settings"));

        // Saving must not drop the front matter, archive or settings block
        const QString saved = MarkdownIO::serialize(p);
        QCOMPARE(saved, original);
    }

    // ── Filenames ─────────────────────────────────────────────────────────────

    void loadAll_nameWithDots_keepsFullName()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        MarkdownIO::save(dir.path(), Project{"v1.2 release.plan", {}});

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects.size(), 1);
        QCOMPARE(projects[0].name, QString("v1.2 release.plan"));
    }

    void save_createsMissingDirectory()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString sub = dir.path() + "/a/b";

        QVERIFY(MarkdownIO::save(sub, Project{"P", {}}));
        QCOMPARE(MarkdownIO::loadAll(sub).size(), 1);
    }

    void save_unwritableLocation_returnsFalse()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        // A regular file where the directory should be
        QFile blocker(dir.path() + "/notadir");
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();

        QVERIFY(!MarkdownIO::save(dir.path() + "/notadir", Project{"P", {}}));
    }

    void rename_missingSource_succeeds()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(MarkdownIO::rename(dir.path(), "Nope", "Other"));
    }

    void rename_existingTarget_failsAndKeepsBothFiles()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        MarkdownIO::save(dir.path(), Project{"A", {Column{"FromA", {}}}});
        MarkdownIO::save(dir.path(), Project{"B", {Column{"FromB", {}}}});

        QVERIFY(!MarkdownIO::rename(dir.path(), "A", "B"));

        const auto projects = MarkdownIO::loadAll(dir.path());
        QCOMPARE(projects.size(), 2);
        QCOMPARE(projects[1].columns[0].name, QString("FromB"));
    }
};

QTEST_GUILESS_MAIN(TestMarkdownIO)
#include "test_markdownio.moc"
