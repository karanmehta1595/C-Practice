// notelinkservice.cpp
#include "notelinkservice.h"

#include <QRegularExpression>

QStringList NoteLinkService::linkedTitles(const QString &text)
{
    QStringList titles;
    QRegularExpression regex(QStringLiteral(R"(\[\[([^\]]+)\]\])"));
    QRegularExpressionMatchIterator it = regex.globalMatch(text);
    while (it.hasNext()) {
        const QString title = it.next().captured(1).trimmed();
        if (!title.isEmpty() && !titles.contains(title, Qt::CaseInsensitive))
            titles.append(title);
    }
    return titles;
}

QString NoteLinkService::linkTitleAt(const QString &text, int cursorPosition)
{
    QRegularExpression regex(QStringLiteral(R"(\[\[([^\]]+)\]\])"));
    QRegularExpressionMatchIterator it = regex.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (cursorPosition >= match.capturedStart(0) && cursorPosition <= match.capturedEnd(0))
            return match.captured(1).trimmed();
    }
    return QString();
}
