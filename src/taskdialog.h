#pragma once

#include <QDialog>
#include <QTextListFormat>
#include "model/types.h"

class QAction;
class QComboBox;
class QLineEdit;
class DescriptionEdit;

/**
 * Modal editor for a task's title and rich-text description.
 *
 * The description is edited with a formatting toolbar and saved as Markdown;
 * pasted images are written to the attachments folder when the dialog is
 * accepted. The caller applies title() / description() to the model.
 */
class TaskDialog : public QDialog
{
    Q_OBJECT

public:
    TaskDialog(const Task &task, const QString &boardsDir, QWidget *parent = nullptr);

    QString title() const;

    /**
     * The description as Markdown. Unchanged text is returned exactly as it
     * was loaded, so merely opening a task never reformats its Markdown.
     */
    QString description() const;

    /** Whether the title or description differs from what was loaded. */
    bool hasChanges() const;

    DescriptionEdit *editor() const { return m_edit; }

public slots:
    void accept() override;

    /** Asks before discarding unsaved changes. */
    void reject() override;

private:
    void buildToolbar(class QToolBar *toolbar);
    void updateFormatActions();

    void toggleBold();
    void toggleItalic();
    void toggleStrikeOut();
    void toggleCode();
    void setHeading(int level);
    void toggleList(QTextListFormat::Style style, bool checkList);
    void editLink();
    void insertImageFromFile();

    QString m_originalTitle;
    QString m_originalDescription;

    QLineEdit       *m_title = nullptr;
    DescriptionEdit *m_edit  = nullptr;
    QComboBox       *m_heading = nullptr;

    QAction *m_bold      = nullptr;
    QAction *m_italic    = nullptr;
    QAction *m_strike    = nullptr;
    QAction *m_code      = nullptr;
    QAction *m_bullets   = nullptr;
    QAction *m_numbers   = nullptr;
    QAction *m_checkList = nullptr;
};
