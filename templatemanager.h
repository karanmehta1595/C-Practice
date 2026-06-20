// templatemanager.h
#ifndef TEMPLATEMANAGER_H
#define TEMPLATEMANAGER_H

#include "notemodel.h"

#include <QString>
#include <QStringList>

struct NoteTemplate
{
    QString name;
    QString title;
    QString body;
    QStringList tags;
};

class TemplateManager
{
public:
    static QList<NoteTemplate> templates();
    static NoteTemplate templateNamed(const QString &name);
};

#endif // TEMPLATEMANAGER_H
