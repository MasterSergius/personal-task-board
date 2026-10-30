#include "descriptionedit.h"

#include <QBuffer>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QTextBlock>
#include <QTextDocument>
#include <QUrl>

#include "model/markdownio.h"

// Image file types kept as-is; anything else is converted to PNG.
static const QStringList keptImageSuffixes = {"png", "jpg", "jpeg", "gif", "webp", "bmp"};

static bool isLocalImageFile(const QUrl &url)
{
    return url.isLocalFile() && !QImageReader::imageFormat(url.toLocalFile()).isEmpty();
}

static bool allLocalImageFiles(const QMimeData *source)
{
    if (!source->hasUrls() || source->urls().isEmpty())
        return false;
    for (const QUrl &url : source->urls())
        if (!isLocalImageFile(url))
            return false;
    return true;
}

// ─── Construction / content ───────────────────────────────────────────────────

DescriptionEdit::DescriptionEdit(const QString &boardsDir, QWidget *parent)
    : QTextEdit(parent)
    , m_boardsDir(boardsDir)
{
    setAcceptRichText(true);
    // Relative image paths in the Markdown resolve against the boards directory.
    document()->setBaseUrl(QUrl::fromLocalFile(QDir(boardsDir).absolutePath() + "/"));
}

void DescriptionEdit::setMarkdownText(const QString &markdown)
{
    m_pendingAttachments.clear();
    document()->setMarkdown(MarkdownIO::withImageAltText(markdown),
                            QTextDocument::MarkdownDialectGitHub);
    fitImages();
    document()->clearUndoRedoStacks();
    document()->setModified(false);
}

QString DescriptionEdit::markdownText() const
{
    return document()->toMarkdown(QTextDocument::MarkdownDialectGitHub).trimmed();
}

// ─── Images ───────────────────────────────────────────────────────────────────

bool DescriptionEdit::insertImage(const QImage &image)
{
    if (image.isNull())
        return false;

    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
        return false;

    insertAttachment(png, "png", image);
    return true;
}

bool DescriptionEdit::insertImageFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray data = file.readAll();
    const QImage image = QImage::fromData(data);
    if (image.isNull())
        return false;

    const QString suffix = QFileInfo(path).suffix().toLower();
    if (keptImageSuffixes.contains(suffix)) {
        insertAttachment(data, suffix, image);
        return true;
    }
    return insertImage(image);
}

void DescriptionEdit::insertAttachment(const QByteArray &data, const QString &extension,
                                       const QImage &image)
{
    // No spaces: Markdown image links don't allow them unescaped.
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString name;
    do {
        const quint32 rnd = QRandomGenerator::global()->generate() & 0xffffff;
        name = QString("%1/%2-%3.%4")
                   .arg(MarkdownIO::AttachmentsDir, stamp)
                   .arg(rnd, 6, 16, QChar('0'))
                   .arg(extension);
    } while (m_pendingAttachments.contains(name) || QFile::exists(m_boardsDir + "/" + name));

    m_pendingAttachments.insert(name, data);
    document()->addResource(QTextDocument::ImageResource, QUrl(name), image);

    QTextImageFormat format;
    format.setName(name);
    if (image.width() > MaxImageWidth)
        format.setWidth(MaxImageWidth);   // height follows, keeping the aspect ratio
    textCursor().insertImage(format);
}

void DescriptionEdit::fitImages()
{
    QList<QPair<QTextCursor, QTextImageFormat>> oversized;

    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.charFormat().isImageFormat())
                continue;

            QTextImageFormat format = fragment.charFormat().toImageFormat();
            if (format.hasProperty(QTextFormat::ImageWidth))
                continue;

            const QVariant res = document()->resource(QTextDocument::ImageResource,
                                                      QUrl(format.name()));
            QImage image;
            if (res.typeId() == QMetaType::QImage)
                image = res.value<QImage>();
            else if (res.typeId() == QMetaType::QPixmap)
                image = res.value<QPixmap>().toImage();
            else if (res.typeId() == QMetaType::QByteArray)
                image = QImage::fromData(res.toByteArray());

            if (image.width() > MaxImageWidth) {
                QTextCursor cursor(document());
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
                format.setWidth(MaxImageWidth);
                oversized.append({cursor, format});
            }
        }
    }

    // Applied after the walk: changing formats would invalidate the iterators.
    for (auto &[cursor, format] : oversized)
        cursor.setCharFormat(format);
}

bool DescriptionEdit::saveAttachments(QString *error)
{
    if (m_pendingAttachments.isEmpty())
        return true;

    // Images removed again before saving are simply not written.
    const QString markdown = markdownText();
    const QString dir = m_boardsDir + "/" + MarkdownIO::AttachmentsDir;

    for (auto it = m_pendingAttachments.begin(); it != m_pendingAttachments.end();) {
        if (!markdown.contains(it.key())) {
            ++it;
            continue;
        }

        if (!QDir().mkpath(dir)) {
            if (error)
                *error = QString("Could not create folder %1").arg(dir);
            return false;
        }

        QSaveFile file(m_boardsDir + "/" + it.key());
        if (!file.open(QIODevice::WriteOnly) || file.write(it.value()) != it.value().size()
            || !file.commit()) {
            if (error)
                *error = QString("Could not write %1: %2").arg(file.fileName(), file.errorString());
            return false;
        }
        it = m_pendingAttachments.erase(it);
    }
    return true;
}

// ─── Paste / drop ─────────────────────────────────────────────────────────────

bool DescriptionEdit::canInsertFromMimeData(const QMimeData *source) const
{
    return source->hasImage() || allLocalImageFiles(source)
        || QTextEdit::canInsertFromMimeData(source);
}

void DescriptionEdit::insertFromMimeData(const QMimeData *source)
{
    // Image files from a file manager: copy them in.
    if (allLocalImageFiles(source)) {
        for (const QUrl &url : source->urls())
            insertImageFile(url.toLocalFile());
        return;
    }

    // Image data (screenshots, "Copy Image" in a browser). Preferred over
    // the HTML that often comes along, whose <img> points at a remote URL.
    if (source->hasImage() && insertImage(qvariant_cast<QImage>(source->imageData())))
        return;

    QTextEdit::insertFromMimeData(source);
}

// ─── Mouse: check boxes and links ─────────────────────────────────────────────

bool DescriptionEdit::toggleCheckBoxAt(const QPoint &pos)
{
    const QTextCursor hit = cursorForPosition(pos);
    QTextBlock block = hit.block();
    QTextBlockFormat format = block.blockFormat();
    if (format.marker() == QTextBlockFormat::MarkerType::NoMarker)
        return false;

    // The box is drawn left of the item's text, on its first line.
    QTextCursor start(block);
    const QRect textStart = cursorRect(start);
    if (pos.x() >= textStart.left() || pos.y() < textStart.top() || pos.y() > textStart.bottom())
        return false;

    format.setMarker(format.marker() == QTextBlockFormat::MarkerType::Checked
                         ? QTextBlockFormat::MarkerType::Unchecked
                         : QTextBlockFormat::MarkerType::Checked);
    start.setBlockFormat(format);
    return true;
}

void DescriptionEdit::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier
        && toggleCheckBoxAt(event->position().toPoint())) {
        event->accept();
        return;
    }
    QTextEdit::mousePressEvent(event);
}

void DescriptionEdit::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier)) {
        const QString href = anchorAt(event->position().toPoint());
        if (!href.isEmpty()) {
            QDesktopServices::openUrl(document()->baseUrl().resolved(QUrl(href)));
            event->accept();
            return;
        }
    }
    QTextEdit::mouseReleaseEvent(event);
}
