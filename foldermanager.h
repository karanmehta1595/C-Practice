// foldermanager.h
#ifndef FOLDERMANAGER_H
#define FOLDERMANAGER_H

#include "notemodel.h"

#include <QString>
#include <QStringList>

class FolderManager
{
public:
    static QString normalizedName(const QString &name);
    static QString folderPath(const QList<NoteFolder> &folders, const QString &folderId);
    static QString displayName(const QList<NoteFolder> &folders, const NoteFolder &folder);
    static QStringList descendantFolderIds(const QList<NoteFolder> &folders, const QString &folderId);
    static QList<NoteFolder> sortedForDisplay(const QList<NoteFolder> &folders);
    static bool isDescendantOf(const QList<NoteFolder> &folders,
                               const QString &folderId,
                               const QString &possibleAncestorId);
};

#endif // FOLDERMANAGER_H
