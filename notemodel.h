// notemodel.h
#ifndef NOTEMODEL_H
#define NOTEMODEL_H

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

struct NoteAttachment
{
    QString id;
    QString filePath;
    QString displayName;
    QString mimeType;
    qint64 size = 0;
    QDateTime addedAt;

    QJsonObject toJson() const;
    static NoteAttachment fromJson(const QJsonObject &object);
};

struct Note
{
    QString id;
    QString title = QStringLiteral("Untitled Note");
    QString body;
    QString html;
    QString folderId;
    QStringList tags;
    QList<NoteAttachment> attachments;
    bool pinned = false;
    bool trashed = false;
    bool locked = false;
    QString passwordHash;
    QString passwordHint;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime trashedAt;

    QString driveFileId;
    QDateTime lastSyncedAt;

    QJsonObject toJson() const;
    static Note fromJson(const QJsonObject &object);
};

struct NoteFolder
{
    QString id;
    QString name = QStringLiteral("New Folder");
    QString parentId;
    QDateTime createdAt;

    QJsonObject toJson() const;
    static NoteFolder fromJson(const QJsonObject &object);
};

struct SmartFolder
{
    QString id;
    QString name = QStringLiteral("Smart Folder");
    QString builtInKind;
    QStringList requiredTags;
    bool matchAllTags = false;
    bool requirePinned = false;
    bool requireAttachments = false;
    bool requireLocked = false;
    QDateTime createdAt;

    QJsonObject toJson() const;
    static SmartFolder fromJson(const QJsonObject &object);
};

struct NoteMetadata
{
    int wordCount = 0;
    int characterCount = 0;
    int readingTimeMinutes = 0;
};

namespace NoteModel {
QStringList cleanTags(const QStringList &values);
QString htmlToPlainText(const QString &html);
NoteMetadata metadataForText(const QString &text);
}

#endif // NOTEMODEL_H
