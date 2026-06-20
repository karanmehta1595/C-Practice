// notemodel.cpp
#include "notemodel.h"

#include <QRegularExpression>
#include <QSet>
#include <QTextDocument>

namespace NoteModel {

QStringList cleanTags(const QStringList &values)
{
    QSet<QString> seen;
    QStringList result;
    for (QString value : values) {
        value = value.trimmed();
        if (value.startsWith(QLatin1Char('#')))
            value.remove(0, 1);
        if (value.isEmpty())
            continue;
        const QString key = value.toLower();
        if (seen.contains(key))
            continue;
        seen.insert(key);
        result.append(value);
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

QString htmlToPlainText(const QString &html)
{
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText();
}

NoteMetadata metadataForText(const QString &text)
{
    NoteMetadata metadata;
    metadata.characterCount = text.size();
    const QString trimmed = text.trimmed();
    if (!trimmed.isEmpty()) {
        metadata.wordCount = trimmed.split(QRegularExpression(QStringLiteral("\\s+")),
                                           Qt::SkipEmptyParts).size();
    }
    metadata.readingTimeMinutes = qMax(1, (metadata.wordCount + 199) / 200);
    if (metadata.wordCount == 0)
        metadata.readingTimeMinutes = 0;
    return metadata;
}

}
