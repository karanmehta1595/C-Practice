// smartfoldermanager.h
#ifndef SMARTFOLDERMANAGER_H
#define SMARTFOLDERMANAGER_H

#include "notemodel.h"

#include <QString>

class SmartFolderManager
{
public:
    static QString recentKind();
    static QString pinnedKind();
    static QString attachmentsKind();
    static QString createdTodayKind();
    static QString modifiedTodayKind();
    static QString modifiedThisWeekKind();
    static QString untaggedKind();
    static QString lockedKind();

    static bool matchesBuiltIn(const Note &note, const QString &kind);
    static QList<Note> filterBuiltIn(const QList<Note> &notes, const QString &kind);
    static bool matchesCustomSmartFolder(const Note &note, const SmartFolder &folder);
};

#endif // SMARTFOLDERMANAGER_H
