// attachmentmanager.h
#ifndef ATTACHMENTMANAGER_H
#define ATTACHMENTMANAGER_H

#include "notemodel.h"

#include <QImage>
#include <QString>
#include <QStringList>

class AttachmentManager
{
public:
    static bool isImage(const NoteAttachment &attachment);
    static bool isPdf(const NoteAttachment &attachment);
    static bool isGeneralFile(const NoteAttachment &attachment);
    static QList<Note> notesWithImages(const QList<Note> &notes);
    static QList<Note> notesWithPdfs(const QList<Note> &notes);
    static QList<Note> notesWithGeneralFiles(const QList<Note> &notes);
    static QString savePastedImage(const QString &storageDirectory, const QImage &image);
};

#endif // ATTACHMENTMANAGER_H
