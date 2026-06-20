// foldermanager.cpp
#include "foldermanager.h"

#include <QSet>

#include <algorithm>

QString FolderManager::normalizedName(const QString &name)
{
    const QString clean = name.trimmed();
    return clean.isEmpty() ? QStringLiteral("New Folder") : clean;
}

QString FolderManager::folderPath(const QList<NoteFolder> &folders, const QString &folderId)
{
    QStringList parts;
    QString currentId = folderId;
    QSet<QString> visited;

    while (!currentId.isEmpty() && !visited.contains(currentId)) {
        visited.insert(currentId);
        auto it = std::find_if(folders.begin(), folders.end(), [&currentId](const NoteFolder &folder) {
            return folder.id == currentId;
        });
        if (it == folders.end())
            break;
        parts.prepend(it->name);
        currentId = it->parentId;
    }

    return parts.join(QStringLiteral(" / "));
}

QString FolderManager::displayName(const QList<NoteFolder> &folders, const NoteFolder &folder)
{
    const QString path = folderPath(folders, folder.id);
    return path.isEmpty() ? folder.name : path;
}

QStringList FolderManager::descendantFolderIds(const QList<NoteFolder> &folders, const QString &folderId)
{
    QStringList result;
    for (const NoteFolder &folder : folders) {
        if (folder.parentId == folderId) {
            result.append(folder.id);
            result.append(descendantFolderIds(folders, folder.id));
        }
    }
    return result;
}

QList<NoteFolder> FolderManager::sortedForDisplay(const QList<NoteFolder> &folders)
{
    QList<NoteFolder> sorted = folders;
    std::sort(sorted.begin(), sorted.end(), [&folders](const NoteFolder &left, const NoteFolder &right) {
        return displayName(folders, left).compare(displayName(folders, right), Qt::CaseInsensitive) < 0;
    });
    return sorted;
}

bool FolderManager::isDescendantOf(const QList<NoteFolder> &folders,
                                   const QString &folderId,
                                   const QString &possibleAncestorId)
{
    QString currentId = folderId;
    QSet<QString> visited;
    while (!currentId.isEmpty() && !visited.contains(currentId)) {
        visited.insert(currentId);
        auto it = std::find_if(folders.begin(), folders.end(), [&currentId](const NoteFolder &folder) {
            return folder.id == currentId;
        });
        if (it == folders.end())
            return false;
        if (it->parentId == possibleAncestorId)
            return true;
        currentId = it->parentId;
    }
    return false;
}
