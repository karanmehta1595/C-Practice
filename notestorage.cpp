// notestorage.cpp
#include "notestorage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {
constexpr int kSchemaVersion = 3;

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage)
        *errorMessage = message;
}
}

NoteStorage::NoteStorage(const QString &storageDirectory)
    : m_storageDirectory(storageDirectory.isEmpty()
                             ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                             : storageDirectory)
{
}

QString NoteStorage::storageDirectory() const
{
    return m_storageDirectory;
}

void NoteStorage::setStorageDirectory(const QString &directoryPath)
{
    if (!directoryPath.trimmed().isEmpty())
        m_storageDirectory = directoryPath;
}

QString NoteStorage::libraryFilePath() const
{
    return QDir(m_storageDirectory).filePath(QStringLiteral("library.json"));
}

bool NoteStorage::load(NotesLibraryData *data, QString *errorMessage) const
{
    if (!data) {
        setError(errorMessage, QStringLiteral("No library container was provided."));
        return false;
    }

    QDir dir(m_storageDirectory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        setError(errorMessage, QStringLiteral("Could not create the notes storage folder."));
        return false;
    }

    QFile file(libraryFilePath());
    if (!file.exists()) {
        *data = NotesLibraryData();
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("Could not open the notes library."));
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QStringLiteral("The notes library JSON is invalid."));
        return false;
    }

    const QJsonObject root = document.object();
    NotesLibraryData next;

    const QJsonArray notesArray = root.value(QStringLiteral("notes")).toArray();
    next.notes.reserve(notesArray.size());
    for (const QJsonValue &value : notesArray)
        next.notes.append(Note::fromJson(value.toObject()));

    const QJsonArray foldersArray = root.value(QStringLiteral("folders")).toArray();
    next.folders.reserve(foldersArray.size());
    for (const QJsonValue &value : foldersArray)
        next.folders.append(NoteFolder::fromJson(value.toObject()));

    const QJsonArray smartArray = root.value(QStringLiteral("smartFolders")).toArray();
    next.smartFolders.reserve(smartArray.size());
    for (const QJsonValue &value : smartArray)
        next.smartFolders.append(SmartFolder::fromJson(value.toObject()));

    const QJsonArray recentArray = root.value(QStringLiteral("recent")).toArray();
    for (const QJsonValue &value : recentArray) {
        const QString id = value.toString();
        if (!id.isEmpty() && !next.recentNoteIds.contains(id))
            next.recentNoteIds.append(id);
    }

    *data = next;
    return true;
}

bool NoteStorage::save(const NotesLibraryData &data, QString *errorMessage) const
{
    QDir dir(m_storageDirectory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        setError(errorMessage, QStringLiteral("Could not create the notes storage folder."));
        return false;
    }

    QJsonArray notesArray;
    for (const Note &note : data.notes)
        notesArray.append(note.toJson());

    QJsonArray foldersArray;
    for (const NoteFolder &folder : data.folders)
        foldersArray.append(folder.toJson());

    QJsonArray smartArray;
    for (const SmartFolder &folder : data.smartFolders)
        smartArray.append(folder.toJson());

    QJsonArray recentArray;
    for (const QString &id : data.recentNoteIds)
        recentArray.append(id);

    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    root.insert(QStringLiteral("notes"), notesArray);
    root.insert(QStringLiteral("folders"), foldersArray);
    root.insert(QStringLiteral("smartFolders"), smartArray);
    root.insert(QStringLiteral("recent"), recentArray);

    const QString finalPath = libraryFilePath();
    const QString tempPath = finalPath + QStringLiteral(".tmp");

    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setError(errorMessage, QStringLiteral("Could not write the notes library."));
        return false;
    }

    tempFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    tempFile.close();

    QFile::remove(finalPath);
    if (!QFile::rename(tempPath, finalPath)) {
        setError(errorMessage, QStringLiteral("Could not replace the notes library."));
        return false;
    }

    return true;
}
