#pragma once

#include <QHash>
#include <QTextEdit>

class QImage;

/**
 * Rich-text editor for a task description, stored as Markdown.
 *
 * Anything Markdown can express survives saving: bold, italic, strikethrough,
 * inline code, headings, bullet / numbered / check lists, links and images.
 *
 * Images pasted or dropped into the editor are kept in memory until
 * saveAttachments() writes them to "<boardsDir>/attachments/", so cancelling
 * the edit leaves no stray files behind. They are referenced from the
 * Markdown by a path relative to the boards directory.
 *
 * Clicking a check-list box toggles it; Ctrl+click opens a link.
 */
class DescriptionEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit DescriptionEdit(const QString &boardsDir, QWidget *parent = nullptr);

    /** Replace the content with the given Markdown (not undoable, not "modified"). */
    void setMarkdownText(const QString &markdown);

    /** The current content as Markdown. */
    QString markdownText() const;

    /** Insert an image as a new attachment. Returns false if it is null. */
    bool insertImage(const QImage &image);

    /** Copy an image file into the description. Returns false if it isn't a readable image. */
    bool insertImageFile(const QString &path);

    /**
     * Write the pasted images that are still referenced to the attachments
     * folder. Returns false (with *error set) if a file could not be written.
     */
    bool saveAttachments(QString *error = nullptr);

    /** Images displayed wider than this are scaled down to fit. */
    static constexpr int MaxImageWidth = 520;

protected:
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /** Registers the image under a new attachment name and inserts it at the cursor. */
    void insertAttachment(const QByteArray &data, const QString &extension, const QImage &image);

    /** Scales images loaded from disk down to MaxImageWidth. */
    void fitImages();

    /** Toggles the check box at pos if there is one. Returns true if toggled. */
    bool toggleCheckBoxAt(const QPoint &pos);

    QString m_boardsDir;
    QHash<QString, QByteArray> m_pendingAttachments;   // relative path → file content
};
