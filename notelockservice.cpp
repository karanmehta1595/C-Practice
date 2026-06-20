// notelockservice.cpp
#include "notelockservice.h"

#include <QCryptographicHash>

QString NoteLockService::hashPassword(const QString &password)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool NoteLockService::passwordMatches(const QString &password, const QString &hash)
{
    return !password.isEmpty() && hashPassword(password) == hash;
}
