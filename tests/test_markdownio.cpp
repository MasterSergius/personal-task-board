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

    // ── Priority / color fields ───────────────────────────────────────────────

    void parse_priorityAndColorFields()
    {
        const Project p = MarkdownIO::parse(
            "## Col\n- [ ] Fix login [priority:: high] [color:: #E5484D]\n- [ ] Plain\n", "P");
        const auto &tasks = p.columns[0].tasks;
        QCOMPARE(tasks[0].title,    QString("Fix login"));
        QCOMPARE(tasks[0].priority, Priority::High);
        QCOMPARE(tasks[0].color,    QString("#e5484d"));
        QCOMPARE(tasks[1].priority, Priority::None);
        QVERIFY(tasks[1].color.isEmpty());
    }

    void parse_fields_anyOrderCaseAndPosition()
    {
        const Project p = MarkdownIO::parse(
            "## Col\n"
            "- [x] [Color::#abc] [PRIORITY:: Urgent] Ship it\n"
            "- [ ] Before [priority:: low] after\n", "P");
        const auto &tasks = p.columns[0].tasks;
        QCOMPARE(tasks[0].title,    QString("Ship it"));
        QCOMPARE(tasks[0].done,     true);
        QCOMPARE(tasks[0].priority, Priority::Urgent);
        QCOMPARE(tasks[0].color,    QString("#aabbcc"));
        QCOMPARE(tasks[1].title,    QString("Before after"));
        QCOMPARE(tasks[1].priority, Priority::Low);
    }

    void parse_unrecognisedFields_stayInTitle()
    {
        const QString title = "T [priority:: someday] [color:: red] [due:: 2026-11-01]";
        const Project p = MarkdownIO::parse("## Col\n- [ ] " + title + "\n", "P");
        const Task &t = p.columns[0].tasks[0];
        QCOMPARE(t.title,    title);
        QCOMPARE(t.priority, Priority::None);
        QVERIFY(t.color.isEmpty());
    }

    void parse_duplicateField_firstWinsRestKept()
    {
        const Project p = MarkdownIO::parse(
            "## Col\n- [ ] T [priority:: high] [priority:: low]\n", "P");
        const Task &t = p.columns[0].tasks[0];
        QCOMPARE(t.priority, Priority::High);
        QCOMPARE(t.title,    QString("T [priority:: low]"));
    }

    void serialize_writesFieldsAfterTitle()
    {
        Task t{"Fix login"};
        t.priority = Priority::High;
        t.color    = "#e5484d";
        Task onlyColor{"Docs"};
        onlyColor.color = "#30a46c";

        const Project p{"P", {Column{"Col", {t, onlyColor, Task{"Plain"}}}}};
        QCOMPARE(MarkdownIO::serialize(p),
                 QString("## Col\n\n"
                         "- [ ] Fix login [priority:: high] [color:: #e5484d]\n"
                         "- [ ] Docs [color:: #30a46c]\n"
                         "- [ ] Plain\n"));
    }

    void priorityAndColor_roundTripThroughDisk()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Task t{"Task", true};
        t.priority = Priority::Medium;
        t.color    = "#8e4ec6";
        QVERIFY(MarkdownIO::save(dir.path(), Project{"P", {Column{"Col", {t}}}}));

        const Task got = MarkdownIO::loadAll(dir.path())[0].columns[0].tasks[0];
        QCOMPARE(got.title,    QString("Task"));
        QCOMPARE(got.done,     true);
        QCOMPARE(got.priority, Priority::Medium);
        QCOMPARE(got.color,    QString("#8e4ec6"));
    }

    void priorityFromName()
    {
        QCOMPARE(MarkdownIO::priorityFromName("low"),      std::optional(Priority::Low));
        QCOMPARE(MarkdownIO::priorityFromName(" Medium "), std::optional(Priority::Medium));
        QCOMPARE(MarkdownIO::priorityFromName("HIGH"),     std::optional(Priority::High));
        QCOMPARE(MarkdownIO::priorityFromName("urgent"),   std::optional(Priority::Urgent));
        QVERIFY(!MarkdownIO::priorityFromName("none"));
        QVERIFY(!MarkdownIO::priorityFromName(""));
        QVERIFY(!MarkdownIO::priorityFromName("critical"));
    }

    void normalizeColor_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");

        QTest::newRow("lower6")   << QString("#e5484d")  << QString("#e5484d");
        QTest::newRow("upper6")   << QString("#E5484D")  << QString("#e5484d");
        QTest::newRow("short")    << QString("#AbC")     << QString("#aabbcc");
        QTest::newRow("padded")   << QString(" #123456 ") << QString("#123456");
        QTest::newRow("noHash")   << QString("e5484d")   << QString();
        QTest::newRow("name")     << QString("red")      << QString();
        QTest::newRow("badHex")   << QString("#ggg")     << QString();
        QTest::newRow("alpha")    << QString("#11223344") << QString();
        QTest::newRow("empty")    << QString()           << QString();
    }

    void normalizeColor()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QCOMPARE(MarkdownIO::normalizeColor(input), expected);
    }
    // ── Descriptions ──────────────────────────────────────────────────────────

    void parse_description_indentedLinesBelongToTask()
    {
        const Project p = MarkdownIO::parse(
            "## Col\n\n"
            "- [ ] A [priority:: high]\n"
            "    First **line**\n"
            "\n"
            "    Second paragraph\n"
            "    - [ ] sub\n"
            "      - nested\n"
            "- [x] B\n", "P");

        QCOMPARE(p.columns[0].tasks.size(), 2);
        const Task &a = p.columns[0].tasks[0];
        QCOMPARE(a.title,    QString("A"));
        QCOMPARE(a.priority, Priority::High);
        QCOMPARE(a.description,
                 QString("First **line**\n\nSecond paragraph\n- [ ] sub\n  - nested"));
        QVERIFY(p.columns[0].tasks[1].description.isEmpty());
        QCOMPARE(p.columns[0].tasks[1].done, true);
    }

    void parse_description_blankLinesAloneAreNotDescription()
    {
        const Project p = MarkdownIO::parse("## Col\n\n- [ ] A\n\n\n- [ ] B\n\n## Next\n\n- [ ] C\n", "P");
        QCOMPARE(p.columns.size(), 2);
        QCOMPARE(p.columns[0].tasks.size(), 2);
        for (const Column &c : p.columns)
            for (const Task &t : c.tasks)
                QVERIFY2(t.description.isEmpty(), qPrintable(t.title));
    }

    // Obsidian and hand-written files indent with a tab or two spaces.
    void parse_description_otherIndentation()
    {
        const Project p = MarkdownIO::parse(
            "## Col\n- [ ] Tab\n\tline one\n\t\tdeeper\n- [ ] Two\n  two\n    nested\n", "P");
        QCOMPARE(p.columns[0].tasks[0].description, QString("line one\n\tdeeper"));
        QCOMPARE(p.columns[0].tasks[1].description, QString("two\n  nested"));
    }

    // Only one level is removed, so a code block indented further keeps it.
    void parse_description_keepsDeeperIndent()
    {
        const Project p = MarkdownIO::parse("## Col\n- [ ] A\n        code\n        more\n", "P");
        QCOMPARE(p.columns[0].tasks[0].description, QString("    code\n    more"));
    }

    // Indented lines that look like headings, separators or tasks belong to
    // the description; they must not start a column, the footer or a task.
    void parse_description_specialLinesStayInDescription()
    {
        const Project p = MarkdownIO::parse(
            "## Col\n- [ ] A\n    ## not a column\n    ***\n    - [ ] not a task\n"
            "%% kanban:settings\n%%\n", "P");
        QCOMPARE(p.columns.size(), 1);
        QCOMPARE(p.columns[0].tasks.size(), 1);
        QCOMPARE(p.columns[0].tasks[0].description,
                 QString("## not a column\n***\n- [ ] not a task"));
        QVERIFY(p.footer.startsWith("%% kanban:settings"));
    }

    // Indented lines used to be dropped silently, losing them on the next save.
    void parse_indentedLinesUnderTask_surviveSave()
    {
        const QString text = "## Col\n\n- [ ] A\n    note\n";
        QCOMPARE(MarkdownIO::serialize(MarkdownIO::parse(text, "P")), text);
    }

    void serialize_description_isIndented()
    {
        Task t{"A"};
        t.priority    = Priority::Low;
        t.description = "Line 1\n\n  - item\n";
        const Project p{"P", {Column{"Col", {t, Task{"B"}}}}};

        QCOMPARE(MarkdownIO::serialize(p),
                 QString("## Col\n\n"
                         "- [ ] A [priority:: low]\n"
                         "    Line 1\n"
                         "\n"
                         "      - item\n"
                         "- [ ] B\n"));
    }

    void description_roundTripThroughDisk()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString description =
            "# Heading\n\n"
            "Some *text* with a [link](https://example.org).\n\n"
            "- [x] done\n- [ ] todo\n\n"
            "![](attachments/20261030-120000-abcdef.png)\n\n"
            "```\nindented:\n    code\n```";
        Task t{"Task", false, Priority::Urgent, "#e5484d", description};
        MarkdownIO::save(dir.path(), Project{"P", {Column{"Col", {t}}, Column{"Done", {Task{"X"}}}}});

        const auto projects = MarkdownIO::loadAll(dir.path());
        const Task &loaded = projects[0].columns[0].tasks[0];
        QCOMPARE(loaded.title,       QString("Task"));
        QCOMPARE(loaded.priority,    Priority::Urgent);
        QCOMPARE(loaded.color,       QString("#e5484d"));
        QCOMPARE(loaded.description, description);
        QCOMPARE(projects[0].columns[1].tasks[0].title, QString("X"));
    }

    void description_crlfFile()
    {
        const Project p = MarkdownIO::parse("## Col\r\n- [ ] A\r\n    one\r\n\r\n    two\r\n", "P");
        QCOMPARE(p.columns[0].tasks[0].description, QString("one\n\ntwo"));
    }

    void obsidianBoard_withDescriptions_roundTripsExactly()
    {
        const QString original =
            "---\n\nkanban-plugin: board\n\n---\n\n"
            "## Todo\n\n- [ ] One\n    details\n\n    more\n\n"
            "## Done\n\n- [x] Two\n\n"
            "***\n\n## Archive\n\n- [x] Old\n\n"
            "%% kanban:settings\n```\n{\"kanban-plugin\":\"board\"}\n```\n%%\n";

        const Project p = MarkdownIO::parse(original, "Obs");
        QCOMPARE(p.columns[0].tasks[0].description, QString("details\n\nmore"));
        QCOMPARE(MarkdownIO::serialize(p), original);
    }
    void withImageAltText()
    {
        QCOMPARE(MarkdownIO::withImageAltText("a ![](x.png) b ![ ](y.png) ![alt](z.png)"),
                 QString("a ![image](x.png) b ![image](y.png) ![alt](z.png)"));
        QCOMPARE(MarkdownIO::withImageAltText("[](link) and ![[embed.png]]"),
                 QString("[](link) and ![[embed.png]]"));
    }
};

QTEST_GUILESS_MAIN(TestMarkdownIO)
#include "test_markdownio.moc"
