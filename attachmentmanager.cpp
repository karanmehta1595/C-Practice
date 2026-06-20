// attachmentmanager.cpp
#include "attachmentmanager.h"

#include <QDateTime>
#include <QDir>

namespace {
QList<Note> filterAttachments(const QList<Note> &notes, bool (*predicate)(const NoteAttachment &))
{
    QList<Note> result;
    for (const Note &note : notes) {
        for (const NoteAttachment &attachment : note.attachments) {
            if (predicate(attachment)) {
                result.append(note);
                break;
            }
        }
    }
    return result;
}
}

bool AttachmentManager::isImage(const NoteAttachment &attachment)
{
    return attachment.mimeType.startsWith(QStringLiteral("image/"), Qt::CaseInsensitive);
}

bool AttachmentManager::isPdf(const NoteAttachment &attachment)
{
    return attachment.mimeType.compare(QStringLiteral("application/pdf"), Qt::CaseInsensitive) == 0
           || attachment.displayName.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive);
}

bool AttachmentManager::isGeneralFile(const NoteAttachment &attachment)
{
    return !isImage(attachment) && !isPdf(attachment);
}

QList<Note> AttachmentManager::notesWithImages(const QList<Note> &notes)
{
    return filterAttachments(notes, AttachmentManager::isImage);
}

QList<Note> AttachmentManager::notesWithPdfs(const QList<Note> &notes)
{
    return filterAttachments(notes, AttachmentManager::isPdf);
}

QList<Note> AttachmentManager::notesWithGeneralFiles(const QList<Note> &notes)
{
    return filterAttachments(notes, AttachmentManager::isGeneralFile);
}

QString AttachmentManager::savePastedImage(const QString &storageDirectory, const QImage &image)
{
    if (image.isNull())
        return QString();

    QDir dir(storageDirectory);
    if (!dir.exists(QStringLiteral("Attachments")))
        dir.mkpath(QStringLiteral("Attachments"));

    const QString fileName = QStringLiteral("pasted-image-%1.png")
                                 .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-hhmmss-zzz")));
    const QString path = dir.filePath(QStringLiteral("Attachments/%1").arg(fileName));
    return image.save(path, "PNG") ? path : QString();
}
