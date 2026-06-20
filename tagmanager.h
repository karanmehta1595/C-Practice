// tagmanager.h
#ifndef TAGMANAGER_H
#define TAGMANAGER_H

#include "notemodel.h"

#include <QStringList>

class TagManager
{
public:
    static QStringList defaultTags();
    static QStringList normalizeTags(const QStringList &tags);
    static QStringList mergeWithDefaults(const QStringList &tags);
    static QStringList parseTagQuery(const QString &query);
    static bool noteMatchesTags(const Note &note, const QStringList &tags, bool matchAll = true);
};

#endif // TAGMANAGER_H
