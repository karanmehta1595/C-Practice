// tagmanager.cpp
#include "tagmanager.h"

#include <QRegularExpression>

QStringList TagManager::defaultTags()
{
    return {
        QStringLiteral("Work"),
        QStringLiteral("Personal"),
        QStringLiteral("Finance"),
        QStringLiteral("Ideas")
    };
}

QStringList TagManager::normalizeTags(const QStringList &tags)
{
    return NoteModel::cleanTags(tags);
}

QStringList TagManager::mergeWithDefaults(const QStringList &tags)
{
    return normalizeTags(defaultTags() + tags);
}

QStringList TagManager::parseTagQuery(const QString &query)
{
    QStringList tags;
    QRegularExpression regex(QStringLiteral(R"(#([A-Za-z0-9_-]+))"));
    QRegularExpressionMatchIterator it = regex.globalMatch(query);
    while (it.hasNext()) {
        const QString tag = it.next().captured(1).trimmed();
        if (!tag.isEmpty())
            tags.append(tag);
    }
    return normalizeTags(tags);
}

bool TagManager::noteMatchesTags(const Note &note, const QStringList &tags, bool matchAll)
{
    const QStringList clean = normalizeTags(tags);
    if (clean.isEmpty())
        return true;

    bool matchedAny = false;
    for (const QString &tag : clean) {
        const bool hasTag = note.tags.contains(tag, Qt::CaseInsensitive);
        if (matchAll && !hasTag)
            return false;
        matchedAny = matchedAny || hasTag;
    }

    return matchAll || matchedAny;
}
