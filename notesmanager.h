// notesmanager.h
#ifndef NOTESMANAGER_H
#define NOTESMANAGER_H

#include "notemodel.h"

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class QTimer;

class NotesManager : public QObject
{
    Q_OBJECT

public:
    explicit NotesManager(QObject *parent = nullptr);
    ~NotesManager() override;

    QString storageDirectory() const;
    void setStorageDirectory(const QString &directoryPath);

    bool load();
    bool save();

    QString createNote(const QString &title = QString(),
                       const QString &body = QString(),
                       const QString &folderId = QString());
    QString duplicateNote(const QString &noteId);
    bool renameNote(const QString &noteId, const QString &newTitle);
    bool updateNoteBody(const QString &noteId, const QString &body);
    bool updateNoteContent(const QString &noteId,
                           const QString &title,
                           const QString &body,
                           const QString &html,
                           const QStringList &tags);
    bool setPinned(const QString &noteId, bool pinned);
    bool moveNoteToFolder(const QString &noteId, const QString &folderId);
    bool setTags(const QString &noteId, const QStringList &tags);
    bool addAttachment(const QString &noteId, const QString &filePath);
    bool removeAttachment(const QString &noteId, const QString &attachmentId);
    bool lockNote(const QString &noteId, const QString &password, const QString &hint = QString());
    bool unlockNote(const QString &noteId, const QString &password) const;
    bool removeLock(const QString &noteId, const QString &password);

    bool deleteNote(const QString &noteId);
    bool restoreNote(const QString &noteId);
    bool purgeNote(const QString &noteId);
    void emptyTrash();

    bool hasNote(const QString &noteId) const;
    Note note(const QString &noteId) const;

    QList<Note> allNotes() const;
    QList<Note> notesInFolder(const QString &folderId) const;
    QList<Note> pinnedNotes() const;
    QList<Note> recentNotes(int limit = 10) const;
    QList<Note> attachmentNotes() const;
    QList<Note> imageAttachmentNotes() const;
    QList<Note> pdfAttachmentNotes() const;
    QList<Note> fileAttachmentNotes() const;
    QList<Note> createdTodayNotes() const;
    QList<Note> modifiedTodayNotes() const;
    QList<Note> modifiedThisWeekNotes() const;
    QList<Note> untaggedNotes() const;
    QList<Note> lockedNotes() const;
    QList<Note> trashedNotes() const;
    QList<Note> notesWithTag(const QString &tag) const;
    QList<Note> notesWithTags(const QStringList &tags, bool matchAll = true) const;
    QList<Note> notesInSmartFolder(const QString &smartFolderId) const;
    QList<Note> notesForBuiltInSmartFolder(const QString &kind) const;
    QList<Note> searchNotes(const QString &query, bool includeTrashed = false) const;

    QStringList allTags() const;
    void renameTag(const QString &oldTag, const QString &newTag);
    void deleteTag(const QString &tag);

    void touchRecent(const QString &noteId);

    QString createFolder(const QString &name, const QString &parentFolderId = QString());
    bool renameFolder(const QString &folderId, const QString &newName);
    bool deleteFolder(const QString &folderId);
    QList<NoteFolder> allFolders() const;
    QString folderDisplayName(const QString &folderId) const;

    QString createSmartFolder(const QString &name,
                              const QStringList &requiredTags,
                              bool matchAllTags,
                              bool requirePinned = false,
                              bool requireAttachments = false,
                              bool requireLocked = false);
    bool updateSmartFolder(const SmartFolder &folder);
    bool deleteSmartFolder(const QString &smartFolderId);
    QList<SmartFolder> allSmartFolders() const;
    SmartFolder smartFolder(const QString &smartFolderId) const;

    bool isAutoSaveEnabled() const;
    void setAutoSaveEnabled(bool enabled);
    int autoSaveIntervalMs() const;
    void setAutoSaveIntervalMs(int intervalMs);
    bool hasUnsavedChanges() const;
    void flushPendingChanges();

    bool isSyncEnabled() const;
    void setSyncEnabled(bool enabled);
    QString noteIdForTitle(const QString &title) const;

signals:
    void noteCreated(const QString &noteId);
    void noteUpdated(const QString &noteId);
    void noteDeleted(const QString &noteId);
    void noteRestored(const QString &noteId);
    void notePurged(const QString &noteId);

    void folderCreated(const QString &folderId);
    void folderRenamed(const QString &folderId);
    void folderDeleted(const QString &folderId);

    void smartFolderCreated(const QString &smartFolderId);
    void smartFolderUpdated(const QString &smartFolderId);
    void smartFolderDeleted(const QString &smartFolderId);

    void dataReloaded();
    void autoSaveStarted();
    void autoSaveFinished(bool success);

    void syncRequested();

private:
    QString libraryFilePath() const;
    QString generateId() const;
    int indexOfNote(const QString &noteId) const;
    int indexOfFolder(const QString &folderId) const;
    int indexOfSmartFolder(const QString &smartFolderId) const;
    void normalizeNote(Note &note) const;
    void markDirty();
    void scheduleAutoSave();

    QString m_storageDir;
    QList<Note> m_notes;
    QList<NoteFolder> m_folders;
    QList<SmartFolder> m_smartFolders;
    QStringList m_recentNoteIds;

    bool m_dirty = false;
    bool m_autoSaveEnabled = true;
    bool m_syncEnabled = false;
    int m_autoSaveIntervalMs = 1500;

    QTimer *m_autoSaveTimer = nullptr;
};

#endif // NOTESMANAGER_H
