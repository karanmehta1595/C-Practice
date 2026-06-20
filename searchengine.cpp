// searchengine.cpp
#include "searchengine.h"

#include "foldermanager.h"
#include "tagmanager.h"

bool SearchEngine::matches(const Note &note,
                           const QString &query,
                           bool includeLockedContent,
                           const QList<NoteFolder> &folders)
{
    const QString needle = query.trimmed();
    if (needle.isEmpty())
        return true;

    const QStringList requiredTags = TagManager::parseTagQuery(needle);
    if (!TagManager::noteMatchesTags(note, requiredTags, true))
        return false;

    if (note.title.contains(needle, Qt::CaseInsensitive)
        || note.tags.join(QLatin1Char(' ')).contains(needle, Qt::CaseInsensitive)) {
        return true;
    }

    for (const QString &tag : requiredTags) {
        if (note.tags.contains(tag, Qt::CaseInsensitive))
            return true;
    }

    const QString folderName = FolderManager::folderPath(folders, note.folderId);
    if (folderName.contains(needle, Qt::CaseInsensitive))
        return true;

    for (const NoteAttachment &attachment : note.attachments) {
        if (attachment.displayName.contains(needle, Qt::CaseInsensitive)
            || attachment.filePath.contains(needle, Qt::CaseInsensitive)
            || attachment.mimeType.contains(needle, Qt::CaseInsensitive)) {
            return true;
        }
    }

    if (note.locked && !includeLockedContent)
        return false;

    return note.body.contains(needle, Qt::CaseInsensitive)
           || note.html.contains(needle, Qt::CaseInsensitive);
}

QList<Note> SearchEngine::filter(const QList<Note> &notes,
                                 const QString &query,
                                 bool includeLockedContent,
                                 const QList<NoteFolder> &folders)
{
    QList<Note> result;
    for (const Note &note : notes) {
        if (matches(note, query, includeLockedContent, folders))
            result.append(note);
    }
    return result;
}
