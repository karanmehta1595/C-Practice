// notelockservice.h
#ifndef NOTELOCKSERVICE_H
#define NOTELOCKSERVICE_H

#include <QString>

class NoteLockService
{
public:
    static QString hashPassword(const QString &password);
    static bool passwordMatches(const QString &password, const QString &hash);
};

#endif // NOTELOCKSERVICE_H
