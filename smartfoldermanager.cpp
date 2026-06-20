// smartfoldermanager.cpp
#include "smartfoldermanager.h"

#include <QDate>

QString SmartFolderManager::recentKind()
{
    return QStringLiteral("recent");
}

QString SmartFolderManager::pinnedKind()
{
    return QStringLiteral("pinned");
}

QString SmartFolderManager::attachmentsKind()
{
    return QStringLiteral("attachments");
}

QString SmartFolderManager::createdTodayKind()
{
    return QStringLiteral("created-today");
}

QString SmartFolderManager::modifiedTodayKind()
{
    return QStringLiteral("modified-today");
}

QString SmartFolderManager::modifiedThisWeekKind()
{
    return QStringLiteral("modified-week");
}

QString SmartFolderManager::untaggedKind()
{
    return QStringLiteral("untagged");
}

QString SmartFolderManager::lockedKind()
{
    return QStringLiteral("locked");
}

bool SmartFolderManager::matchesBuiltIn(const Note &note, const QString &kind)
{
    if (note.trashed)
        return false;

    const QDate today = QDate::currentDate();
    const QDate created = note.createdAt.toLocalTime().date();
    const QDate modified = note.updatedAt.toLocalTime().date();

    if (kind == pinnedKind())
        return note.pinned;
    if (kind == attachmentsKind())
        return !note.attachments.isEmpty();
    if (kind == createdTodayKind())
        return created == today;
    if (kind == modifiedTodayKind())
        return modified == today;
    if (kind == modifiedThisWeekKind())
        return modified >= today.addDays(-6) && modified <= today;
    if (kind == untaggedKind())
        return note.tags.isEmpty();
    if (kind == lockedKind())
        return note.locked;

    return false;
}

QList<Note> SmartFolderManager::filterBuiltIn(const QList<Note> &notes, const QString &kind)
{
    QList<Note> result;
    for (const Note &note : notes) {
        if (matchesBuiltIn(note, kind))
            result.append(note);
    }
    return result;
}

bool SmartFolderManager::matchesCustomSmartFolder(const Note &note, const SmartFolder &folder)
{
    if (note.trashed)
        return false;
    if (folder.requirePinned && !note.pinned)
        return false;
    if (folder.requireAttachments && note.attachments.isEmpty())
        return false;
    if (folder.requireLocked && !note.locked)
        return false;

    if (folder.requiredTags.isEmpty())
        return true;

    bool matchedAny = false;
    for (const QString &tag : folder.requiredTags) {
        const bool hasTag = note.tags.contains(tag, Qt::CaseInsensitive);
        if (folder.matchAllTags && !hasTag)
            return false;
        matchedAny = matchedAny || hasTag;
    }

    return folder.matchAllTags || matchedAny;
}
