// notestorage.h
#ifndef NOTESTORAGE_H
#define NOTESTORAGE_H

#include "notemodel.h"

#include <QString>
#include <QStringList>

struct NotesLibraryData
{
    QList<Note> notes;
    QList<NoteFolder> folders;
    QList<SmartFolder> smartFolders;
    QStringList recentNoteIds;
};

class NoteStorage
{
public:
    explicit NoteStorage(const QString &storageDirectory = QString());

    QString storageDirectory() const;
    void setStorageDirectory(const QString &directoryPath);
    QString libraryFilePath() const;

    bool load(NotesLibraryData *data, QString *errorMessage = nullptr) const;
    bool save(const NotesLibraryData &data, QString *errorMessage = nullptr) const;

private:
    QString m_storageDirectory;
};

#endif // NOTESTORAGE_H
