#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBlock>
#include <QUrl>

#include "cardwidget.h"
#include "columnwidget.h"
#include "descriptionedit.h"
#include "taskdialog.h"
#include "model/appstate.h"

// Task editor: rich-text ↔ Markdown, image attachments, toolbar, card preview.

// Exposes the protected paste/drop entry point.
class TestEdit : public DescriptionEdit
{
public:
    using DescriptionEdit::DescriptionEdit;
    using DescriptionEdit::insertFromMimeData;
};

static QStringList attachmentFiles(const QString &boardsDir)
{
    return QDir(boardsDir + "/attachments").entryList(QDir::Files);
}

static QImage testImage(int w, int h)
{
    QImage img(w, h, QImage::Format_RGB32);
    img.fill(Qt::darkCyan);
    return img;
}

static QAction *toolbarAction(TaskDialog &dlg, const QString &text)
{
    for (QAction *a : dlg.actions())
        if (a->text() == text)
            return a;
    return nullptr;
}

static void selectAll(TaskDialog &dlg)
{
    QTextCursor c = dlg.editor()->textCursor();
    c.select(QTextCursor::Document);
    dlg.editor()->setTextCursor(c);
}

// The document's first image format (an invalid format if there is none).
static QTextImageFormat firstImageFormat(QTextDocument *doc)
{
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next())
        for (auto it = b.begin(); !it.atEnd(); ++it)
            if (it.fragment().charFormat().isImageFormat())
                return it.fragment().charFormat().toImageFormat();
    return {};
}

class TestTaskDialog : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
    }

    // ── DescriptionEdit ───────────────────────────────────────────────────────

    void markdown_roundTripKeepsFormatting()
    {
        DescriptionEdit edit(m_dir->path());
        const QString md =
            "# Plan\n\nSome **bold**, *italic*, ~~gone~~ and `code` "
            "[link](https://example.org).\n\n- [x] done\n- [ ] todo\n\n1.  one\n2.  two";
        edit.setMarkdownText(md);
        QVERIFY(!edit.document()->isModified());

        const QString out = edit.markdownText();
        for (const char *part : {"# Plan", "**bold**", "*italic*", "~~gone~~", "`code`",
                                 "[link](https://example.org)", "- [x] done", "- [ ] todo",
                                 "1.  one"})
            QVERIFY2(out.contains(part), part);
    }

    void pastedImage_isWrittenOnlyOnSave()
    {
        TestEdit edit(m_dir->path());
        QMimeData mime;
        mime.setImageData(testImage(800, 600));
        edit.insertFromMimeData(&mime);

        const QString md = edit.markdownText();
        QVERIFY2(md.contains("](attachments/"), qPrintable(md));
        QVERIFY(md.contains(".png)"));
        QVERIFY(attachmentFiles(m_dir->path()).isEmpty());   // nothing written yet

        // Shown scaled down to fit the editor
        QCOMPARE(firstImageFormat(edit.document()).width(), double(DescriptionEdit::MaxImageWidth));

        QVERIFY(edit.saveAttachments());
        const QStringList files = attachmentFiles(m_dir->path());
        QCOMPARE(files.size(), 1);
        QVERIFY(md.contains("attachments/" + files.first()));
        QVERIFY(!files.first().contains(' '));
        QCOMPARE(QImage(m_dir->path() + "/attachments/" + files.first()).size(), QSize(800, 600));
    }

    void imageRemovedBeforeSave_isNotWritten()
    {
        TestEdit edit(m_dir->path());
        QMimeData mime;
        mime.setImageData(testImage(40, 40));
        edit.insertFromMimeData(&mime);
        edit.undo();

        QVERIFY(!edit.markdownText().contains("attachments/"));
        QVERIFY(edit.saveAttachments());
        QVERIFY(attachmentFiles(m_dir->path()).isEmpty());
    }

    void droppedImageFile_isCopiedKeepingFormat()
    {
        QTemporaryDir elsewhere;
        const QString src = elsewhere.path() + "/photo.jpg";
        QVERIFY(testImage(30, 20).save(src, "JPG"));

        TestEdit edit(m_dir->path());
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(src)});
        edit.insertFromMimeData(&mime);
        QVERIFY(edit.saveAttachments());

        const QStringList files = attachmentFiles(m_dir->path());
        QCOMPARE(files.size(), 1);
        QVERIFY(files.first().endsWith(".jpg"));
        QVERIFY(QFile::exists(src));   // copied, not moved
    }

    void droppedNonImageFile_isNotAnAttachment()
    {
        QTemporaryDir elsewhere;
        const QString src = elsewhere.path() + "/notes.txt";
        QFile f(src);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("hello");
        f.close();

        TestEdit edit(m_dir->path());
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(src)});
        mime.setText(src);
        edit.insertFromMimeData(&mime);

        QVERIFY(edit.saveAttachments());
        QVERIFY(attachmentFiles(m_dir->path()).isEmpty());
        QVERIFY(!edit.markdownText().contains("!["));
    }

    void existingImage_isFittedWithoutMarkingModified()
    {
        QVERIFY(QDir().mkpath(m_dir->path() + "/attachments"));
        QVERIFY(testImage(1200, 300).save(m_dir->path() + "/attachments/big.png"));

        DescriptionEdit edit(m_dir->path());
        edit.setMarkdownText("![](attachments/big.png)");

        QCOMPARE(firstImageFormat(edit.document()).width(), double(DescriptionEdit::MaxImageWidth));
        QVERIFY(!edit.document()->isModified());
        QVERIFY(!edit.document()->isUndoAvailable());
        QVERIFY(edit.markdownText().contains("](attachments/big.png)"));
    }

    void clickingCheckBox_togglesIt()
    {
        DescriptionEdit edit(m_dir->path());
        edit.resize(400, 300);
        edit.setMarkdownText("- [ ] first\n- [ ] second");
        edit.show();
        QVERIFY(QTest::qWaitForWindowExposed(&edit));

        const QTextBlock second = edit.document()->begin().next();
        const QRect textStart = edit.cursorRect(QTextCursor(second));
        const QPoint box(textStart.left() - 8, textStart.center().y());
        QTest::mouseClick(edit.viewport(), Qt::LeftButton, Qt::NoModifier, box);

        QCOMPARE(edit.markdownText(), QString("- [ ] first\n- [x] second"));

        // Clicking the text itself only moves the cursor
        QTest::mouseClick(edit.viewport(), Qt::LeftButton, Qt::NoModifier,
                          textStart.center() + QPoint(10, 0));
        QCOMPARE(edit.markdownText(), QString("- [ ] first\n- [x] second"));
    }

    // ── TaskDialog ────────────────────────────────────────────────────────────

    void unchangedDescription_isReturnedVerbatim()
    {
        // Markdown Qt would rewrite (underscore emphasis, Obsidian embeds, wrapping)
        const QString md = "_emphasis_ and ![[note.png]]\n* star bullet";
        TaskDialog dlg(Task{"T", false, Priority::None, {}, md}, m_dir->path());
        QVERIFY(!dlg.hasChanges());
        QCOMPARE(dlg.description(), md);
        QCOMPARE(dlg.title(), QString("T"));
    }

    void toolbar_formatsAsMarkdown_data()
    {
        QTest::addColumn<QString>("action");
        QTest::addColumn<QString>("expected");

        QTest::newRow("bold")      << "B"           << "**word**";
        QTest::newRow("italic")    << "I"           << "*word*";
        QTest::newRow("strike")    << "S"           << "~~word~~";
        QTest::newRow("code")      << "</>"         << "`word`";
        QTest::newRow("bullets")   << "• List"      << "- word";
        QTest::newRow("numbers")   << "1. List"     << "1.  word";
        QTest::newRow("checklist") << "☐ Checklist" << "- [ ] word";
    }

    void toolbar_formatsAsMarkdown()
    {
        QFETCH(QString, action);
        QFETCH(QString, expected);

        TaskDialog dlg(Task{"T"}, m_dir->path());
        dlg.editor()->setPlainText("word");
        selectAll(dlg);

        QAction *a = toolbarAction(dlg, action);
        QVERIFY2(a, qPrintable(action));
        a->trigger();

        QVERIFY(dlg.hasChanges());
        QCOMPARE(dlg.description(), expected);
    }

    void toolbar_listTogglesOff()
    {
        TaskDialog dlg(Task{"T"}, m_dir->path());
        dlg.editor()->setPlainText("word");
        QAction *checks = toolbarAction(dlg, "☐ Checklist");
        checks->trigger();
        QCOMPARE(dlg.description(), QString("- [ ] word"));
        checks->trigger();
        QCOMPARE(dlg.description(), QString("word"));
    }

    void toolbar_heading()
    {
        TaskDialog dlg(Task{"T"}, m_dir->path());
        dlg.editor()->setPlainText("Title\nbody");
        dlg.editor()->moveCursor(QTextCursor::End);
        auto *combo = dlg.findChild<QComboBox *>();
        QVERIFY(combo);

        emit combo->activated(2);   // Heading 2, applied to the cursor's block
        QCOMPARE(dlg.description(), QString("Title\n\n## body"));
    }

    void accept_writesAttachmentsAndKeepsTitle()
    {
        TaskDialog dlg(Task{"Old"}, m_dir->path());
        dlg.findChild<QLineEdit *>()->setText("  New  ");
        dlg.editor()->insertImage(testImage(20, 20));

        dlg.accept();
        QCOMPARE(dlg.result(), int(QDialog::Accepted));
        QCOMPARE(dlg.title(), QString("New"));
        QCOMPARE(attachmentFiles(m_dir->path()).size(), 1);
        QVERIFY(dlg.description().contains("attachments/" + attachmentFiles(m_dir->path()).first()));
    }

    void emptyTitle_cannotBeSaved()
    {
        TaskDialog dlg(Task{"T"}, m_dir->path());
        dlg.findChild<QLineEdit *>()->setText("   ");

        auto *save = dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save);
        QVERIFY(!save->isEnabled());

        dlg.accept();
        QVERIFY(dlg.result() != QDialog::Accepted);
    }

    void closingWithoutSaving_writesNoAttachments()
    {
        {
            TaskDialog dlg(Task{"T"}, m_dir->path());
            dlg.editor()->insertImage(testImage(20, 20));
        }
        QVERIFY(attachmentFiles(m_dir->path()).isEmpty());
    }

    // ── Card ──────────────────────────────────────────────────────────────────

    void descriptionPreview()
    {
        QCOMPARE(CardWidget::descriptionPreview(""), QString());
        QCOMPARE(CardWidget::descriptionPreview("  \n "), QString());
        QCOMPARE(CardWidget::descriptionPreview("# Head\n\nSome **bold**\ntext"),
                 QString("Head Some bold text"));
        QCOMPARE(CardWidget::descriptionPreview("See ![](attachments/x.png) here"),
                 QString("See [image] here"));

        const QString longText = CardWidget::descriptionPreview(QString("word ").repeated(50), 20);
        QCOMPARE(longText.size(), 20);
        QVERIFY(longText.endsWith(QChar(0x2026)));
    }

    void checkListProgress()
    {
        using P = QPair<int, int>;
        QCOMPARE(CardWidget::checkListProgress(""), P(0, 0));
        QCOMPARE(CardWidget::checkListProgress("- [x] a\n- [ ] b\n  * [X] nested\n1. [ ] num"),
                 P(2, 4));
        QCOMPARE(CardWidget::checkListProgress("not - [x] a list\n- [x]no space\n- plain"),
                 P(0, 0));
    }

    void card_showsPreviewAndProgress()
    {
        Task t{"T"};
        t.description = "Notes\n\n- [x] a\n- [ ] b";
        CardWidget card(t, 0, 0, 0, Theme::light());

        auto *preview = card.findChild<QLabel *>("descriptionPreview");
        QVERIFY(preview);
        QVERIFY(preview->text().startsWith("Notes"));
        auto *progress = card.findChild<QLabel *>("checkListProgress");
        QVERIFY(progress);
        QVERIFY(progress->text().endsWith("1/2"));

        CardWidget plain(Task{"Plain"}, 0, 0, 0, Theme::light());
        QVERIFY(!plain.findChild<QLabel *>("descriptionPreview"));
        QVERIFY(!plain.findChild<QLabel *>("checkListProgress"));
    }

    void card_doubleClick_requestsOpen()
    {
        CardWidget card(Task{"T"}, 0, 2, 5, Theme::light());
        QSignalSpy open(&card, &CardWidget::openRequested);
        QTest::mouseDClick(&card, Qt::LeftButton);
        QCOMPARE(open.size(), 1);
        QCOMPARE(open.first().at(0).toInt(), 2);
        QCOMPARE(open.first().at(1).toInt(), 5);
    }

    void column_forwardsOpenRequest()
    {
        AppState state(m_dir->path());
        state.load();
        state.addProject("P");
        state.addColumn(0, "A");
        state.addColumn(0, "B");
        state.addTask(0, 1, "x");
        state.addTask(0, 1, "y");

        ColumnWidget col(state, 0, 1, Theme::light());
        QSignalSpy open(&col, &ColumnWidget::taskOpenRequested);

        const auto cards = col.findChildren<CardWidget *>();
        QCOMPARE(cards.size(), 2);
        QTest::mouseDClick(cards[1], Qt::LeftButton);
        QCOMPARE(open.size(), 1);
        QCOMPARE(open.first().at(0).toInt(), 1);
        QCOMPARE(open.first().at(1).toInt(), 1);
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
};

QTEST_MAIN(TestTaskDialog)
#include "test_taskdialog.moc"
