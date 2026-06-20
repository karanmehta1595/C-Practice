// searchengine.h
#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include "notemodel.h"

#include <QString>

class SearchEngine
{
public:
    static bool matches(const Note &note,
                        const QString &query,
                        bool includeLockedContent,
                        const QList<NoteFolder> &folders = {});
    static QList<Note> filter(const QList<Note> &notes,
                              const QString &query,
                              bool includeLockedContent,
                              const QList<NoteFolder> &folders = {});
};

#endif // SEARCHENGINE_H
