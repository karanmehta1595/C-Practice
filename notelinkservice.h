// notelinkservice.h
#ifndef NOTELINKSERVICE_H
#define NOTELINKSERVICE_H

#include <QString>
#include <QStringList>

class NoteLinkService
{
public:
    static QStringList linkedTitles(const QString &text);
    static QString linkTitleAt(const QString &text, int cursorPosition);
};

#endif // NOTELINKSERVICE_H
