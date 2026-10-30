#include "taskdialog.h"

#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontInfo>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextList>
#include <QToolBar>
#include <QVBoxLayout>

#include "descriptionedit.h"

// Blocks touched by the cursor's selection (or the cursor's own block).
static QList<QTextBlock> selectedBlocks(const QTextCursor &cursor)
{
    QList<QTextBlock> blocks;
    const QTextDocument *doc = cursor.document();
    const QTextBlock last = doc->findBlock(cursor.selectionEnd());
    for (QTextBlock b = doc->findBlock(cursor.selectionStart()); b.isValid(); b = b.next()) {
        blocks.append(b);
        if (b == last)
            break;
    }
    return blocks;
}

static bool isCheckList(const QTextBlock &block)
{
    return block.blockFormat().marker() != QTextBlockFormat::MarkerType::NoMarker;
}

static bool isCode(const QTextCharFormat &format)
{
    return format.fontFixedPitch() || QFontInfo(format.font()).fixedPitch();
}

// ─── Construction ─────────────────────────────────────────────────────────────

TaskDialog::TaskDialog(const Task &task, const QString &boardsDir, QWidget *parent)
    : QDialog(parent)
    , m_originalTitle(task.title)
    , m_originalDescription(task.description)
{
    setWindowTitle("Task");
    resize(680, 580);

    auto *layout = new QVBoxLayout(this);

    m_title = new QLineEdit(task.title);
    m_title->setPlaceholderText("Task title");
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    layout->addWidget(m_title);

    auto *toolbar = new QToolBar();
    toolbar->setIconSize(QSize(16, 16));
    layout->addWidget(toolbar);

    m_edit = new DescriptionEdit(boardsDir);
    m_edit->setPlaceholderText("Add a description… Paste or drop images, use the toolbar to format.");
    m_edit->setMarkdownText(task.description);
    layout->addWidget(m_edit, 1);

    buildToolbar(toolbar);

    auto *hint = new QLabel("Click a check box to tick it · Ctrl+click opens a link · Ctrl+S saves");
    QFont hintFont = hint->font();
    hintFont.setPointSizeF(hintFont.pointSizeF() * 0.85);
    hint->setFont(hintFont);
    hint->setEnabled(false);   // greyed out

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    QPushButton *saveButton = buttons->button(QDialogButtonBox::Save);
    connect(buttons, &QDialogButtonBox::accepted, this, &TaskDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &TaskDialog::reject);
    connect(m_title, &QLineEdit::textChanged, saveButton, [saveButton](const QString &text) {
        saveButton->setEnabled(!text.trimmed().isEmpty());
    });

    auto *bottomRow = new QHBoxLayout();
    bottomRow->addWidget(hint, 1);
    bottomRow->addWidget(buttons);
    layout->addLayout(bottomRow);

    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this, saveButton] {
        if (saveButton->isEnabled())
            accept();
    });

    connect(m_edit, &QTextEdit::currentCharFormatChanged, this, &TaskDialog::updateFormatActions);
    connect(m_edit, &QTextEdit::cursorPositionChanged,    this, &TaskDialog::updateFormatActions);
    updateFormatActions();

    m_edit->setFocus();
}

void TaskDialog::buildToolbar(QToolBar *toolbar)
{
    auto addToggle = [&](const QString &text, const QString &tip, const QKeySequence &key,
                         void (TaskDialog::*slot)()) {
        QAction *a = toolbar->addAction(text);
        a->setToolTip(key.isEmpty() ? tip : QString("%1 (%2)").arg(tip, key.toString(QKeySequence::NativeText)));
        a->setShortcut(key);
        a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        a->setCheckable(true);
        addAction(a);   // shortcut works anywhere in the dialog
        if (slot)
            connect(a, &QAction::triggered, this, slot);
        return a;
    };

    m_bold   = addToggle("B", "Bold", QKeySequence::Bold, &TaskDialog::toggleBold);
    m_italic = addToggle("I", "Italic", QKeySequence::Italic, &TaskDialog::toggleItalic);
    m_strike = addToggle("S", "Strikethrough", QKeySequence("Ctrl+Shift+X"), &TaskDialog::toggleStrikeOut);
    m_code   = addToggle("</>", "Inline code", QKeySequence("Ctrl+E"), &TaskDialog::toggleCode);

    QFont f = toolbar->font();
    f.setBold(true);
    m_bold->setFont(f);
    f = toolbar->font();
    f.setItalic(true);
    m_italic->setFont(f);
    f = toolbar->font();
    f.setStrikeOut(true);
    m_strike->setFont(f);
    m_code->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    toolbar->addSeparator();

    m_heading = new QComboBox();
    m_heading->addItems({"Paragraph", "Heading 1", "Heading 2", "Heading 3"});
    m_heading->setToolTip("Paragraph style");
    m_heading->setFocusPolicy(Qt::NoFocus);
    toolbar->addWidget(m_heading);
    connect(m_heading, &QComboBox::activated, this, &TaskDialog::setHeading);

    toolbar->addSeparator();

    m_bullets   = addToggle("• List", "Bullet list", QKeySequence("Ctrl+Shift+8"), nullptr);
    m_numbers   = addToggle("1. List", "Numbered list", QKeySequence("Ctrl+Shift+7"), nullptr);
    m_checkList = addToggle("☐ Checklist", "Check list", QKeySequence("Ctrl+Shift+9"), nullptr);
    connect(m_bullets, &QAction::triggered, this,
            [this] { toggleList(QTextListFormat::ListDisc, false); });
    connect(m_numbers, &QAction::triggered, this,
            [this] { toggleList(QTextListFormat::ListDecimal, false); });
    connect(m_checkList, &QAction::triggered, this,
            [this] { toggleList(QTextListFormat::ListDisc, true); });

    toolbar->addSeparator();

    QAction *link = toolbar->addAction("Link…", this, &TaskDialog::editLink);
    link->setShortcut(QKeySequence("Ctrl+K"));
    link->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    link->setToolTip("Add or edit a link (Ctrl+K)");
    addAction(link);

    QAction *image = toolbar->addAction("Image…", this, &TaskDialog::insertImageFromFile);
    image->setToolTip("Insert an image file (you can also paste or drop images)");
}

// ─── Result ───────────────────────────────────────────────────────────────────

QString TaskDialog::title() const
{
    return m_title->text().trimmed();
}

QString TaskDialog::description() const
{
    return m_edit->document()->isModified() ? m_edit->markdownText() : m_originalDescription;
}

void TaskDialog::accept()
{
    if (title().isEmpty())
        return;

    QString error;
    if (!m_edit->saveAttachments(&error)) {
        QMessageBox::warning(this, "Save Task", "Could not save pasted images.\n\n" + error);
        return;
    }
    QDialog::accept();
}

bool TaskDialog::hasChanges() const
{
    return title() != m_originalTitle || m_edit->document()->isModified();
}

void TaskDialog::reject()
{
    if (hasChanges()) {
        const auto answer = QMessageBox::question(
            this, "Discard Changes", "Discard your changes to this task?",
            QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer != QMessageBox::Discard)
            return;
    }
    QDialog::reject();
}

// ─── Toolbar state ────────────────────────────────────────────────────────────

void TaskDialog::updateFormatActions()
{
    const QTextCursor cursor = m_edit->textCursor();
    const QTextCharFormat cf = m_edit->currentCharFormat();

    m_bold->setChecked(cf.fontWeight() >= QFont::Bold);
    m_italic->setChecked(cf.fontItalic());
    m_strike->setChecked(cf.fontStrikeOut());
    m_code->setChecked(isCode(cf));

    m_heading->setCurrentIndex(qBound(0, cursor.blockFormat().headingLevel(), 3));

    const QTextList *list = cursor.currentList();
    const bool checks = list && isCheckList(cursor.block());
    const auto style  = list ? list->format().style() : QTextListFormat::ListStyleUndefined;
    m_checkList->setChecked(checks);
    m_bullets->setChecked(list && !checks && style == QTextListFormat::ListDisc);
    m_numbers->setChecked(list && !checks && style == QTextListFormat::ListDecimal);
}

// ─── Character formats ────────────────────────────────────────────────────────

void TaskDialog::toggleBold()
{
    QTextCharFormat f;
    f.setFontWeight(m_bold->isChecked() ? QFont::Bold : QFont::Normal);
    m_edit->mergeCurrentCharFormat(f);
}

void TaskDialog::toggleItalic()
{
    QTextCharFormat f;
    f.setFontItalic(m_italic->isChecked());
    m_edit->mergeCurrentCharFormat(f);
}

void TaskDialog::toggleStrikeOut()
{
    QTextCharFormat f;
    f.setFontStrikeOut(m_strike->isChecked());
    m_edit->mergeCurrentCharFormat(f);
}

void TaskDialog::toggleCode()
{
    // The Markdown writer emits `code` for fixed-pitch text.
    QTextCharFormat f;
    if (m_code->isChecked()) {
        f.setFontFixedPitch(true);
        f.setFontFamilies(QStringList{QFontDatabase::systemFont(QFontDatabase::FixedFont).family()});
    } else {
        f.setFontFixedPitch(false);
        f.setFontFamilies(QStringList{m_edit->document()->defaultFont().family()});
    }
    m_edit->mergeCurrentCharFormat(f);
}

// ─── Block formats ────────────────────────────────────────────────────────────

void TaskDialog::setHeading(int level)
{
    QTextCursor cursor = m_edit->textCursor();
    cursor.beginEditBlock();

    // Same look as headings loaded from Markdown.
    QTextCharFormat cf;
    cf.setProperty(QTextFormat::FontSizeAdjustment, level > 0 ? 4 - level : 0);
    cf.setFontWeight(level > 0 ? QFont::Bold : QFont::Normal);

    for (const QTextBlock &block : selectedBlocks(cursor)) {
        QTextCursor c(block);
        QTextBlockFormat bf = block.blockFormat();
        bf.setHeadingLevel(level);
        c.setBlockFormat(bf);
        c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        c.mergeCharFormat(cf);
    }
    cursor.endEditBlock();
    m_edit->mergeCurrentCharFormat(cf);   // text typed next continues in the style
    m_edit->setFocus();
}

void TaskDialog::toggleList(QTextListFormat::Style style, bool checkList)
{
    QTextCursor cursor = m_edit->textCursor();
    const QList<QTextBlock> blocks = selectedBlocks(cursor);
    const QTextList *current = cursor.currentList();
    const bool sameKind = current && current->format().style() == style
                       && isCheckList(cursor.block()) == checkList;

    cursor.beginEditBlock();
    if (sameKind) {
        // Toggle off: turn the items back into paragraphs.
        for (const QTextBlock &block : blocks) {
            if (QTextList *list = block.textList())
                list->remove(block);
            QTextBlockFormat bf = block.blockFormat();
            bf.setIndent(0);
            bf.setMarker(QTextBlockFormat::MarkerType::NoMarker);
            QTextCursor(block).setBlockFormat(bf);
        }
    } else {
        QTextListFormat lf;
        lf.setStyle(style);

        QTextList *list = cursor.currentList();
        if (list) {
            list->setFormat(lf);
        } else {
            QTextCursor first(blocks.first());
            list = first.createList(lf);
        }
        for (const QTextBlock &block : blocks) {
            if (block.textList() != list)
                list->add(block);
            QTextBlockFormat bf = block.blockFormat();
            if (!checkList)
                bf.setMarker(QTextBlockFormat::MarkerType::NoMarker);
            else if (bf.marker() == QTextBlockFormat::MarkerType::NoMarker)
                bf.setMarker(QTextBlockFormat::MarkerType::Unchecked);
            QTextCursor(block).setBlockFormat(bf);
        }
    }
    cursor.endEditBlock();
    updateFormatActions();
    m_edit->setFocus();
}

// ─── Links / images ───────────────────────────────────────────────────────────

void TaskDialog::editLink()
{
    QTextCursor cursor = m_edit->textCursor();

    // With no selection inside a link, edit that whole link.
    if (!cursor.hasSelection() && cursor.charFormat().isAnchor()) {
        const QString href = cursor.charFormat().anchorHref();
        const QTextBlock block = cursor.block();
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (frag.charFormat().anchorHref() == href
                && cursor.position() >= frag.position()
                && cursor.position() <= frag.position() + frag.length()) {
                cursor.setPosition(frag.position());
                cursor.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
                break;
            }
        }
    }

    bool ok = false;
    const QString url = QInputDialog::getText(
        this, "Link", "URL (leave empty to remove the link):", QLineEdit::Normal,
        cursor.charFormat().anchorHref(), &ok).trimmed();
    if (!ok)
        return;

    QTextCharFormat link;
    if (url.isEmpty()) {
        link.setAnchor(false);
        link.setAnchorHref(QString());
        link.setFontUnderline(false);
        link.setForeground(m_edit->palette().text());
    } else {
        link.setAnchor(true);
        link.setAnchorHref(url);
        link.setFontUnderline(true);
        link.setForeground(m_edit->palette().link());
    }

    if (cursor.hasSelection()) {
        cursor.mergeCharFormat(link);
    } else if (!url.isEmpty()) {
        QTextCharFormat inserted = cursor.charFormat();
        inserted.merge(link);
        cursor.insertText(url, inserted);

        // Continue typing as plain text after the link.
        QTextCharFormat plain = inserted;
        plain.setAnchor(false);
        plain.clearProperty(QTextFormat::AnchorHref);
        plain.setFontUnderline(false);
        plain.clearForeground();
        m_edit->setTextCursor(cursor);
        m_edit->setCurrentCharFormat(plain);
    }
    m_edit->setFocus();
}

void TaskDialog::insertImageFromFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Insert Image", QString(),
        "Images (*.png *.jpg *.jpeg *.gif *.webp *.bmp *.svg);;All files (*)");
    if (path.isEmpty())
        return;
    if (!m_edit->insertImageFile(path))
        QMessageBox::warning(this, "Insert Image", "This file is not an image Qt can read:\n" + path);
    m_edit->setFocus();
}
