// notesmanager.cpp
#include "notesmanager.h"

#include "attachmentmanager.h"
#include "foldermanager.h"
#include "notelockservice.h"
#include "notestorage.h"
#include "smartfoldermanager.h"
#include "tagmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QSet>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTimer>
#include <QUuid>

//#include <QUuid>

#include <algorithm>

namespace {
constexpr int kMaxRecentNotes = 80;

QString isoString(const QDateTime &dateTime)
{
    return dateTime.isValid() ? dateTime.toUTC().toString(Qt::ISODateWithMs) : QString();
}

QDateTime fromIsoString(const QString &text)
{
    if (text.isEmpty())
        return QDateTime();

    QDateTime dateTime = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (dateTime.isValid())
        dateTime.setTimeZone(QTimeZone::UTC);
    return dateTime;
}

QStringList jsonToStringList(const QJsonArray &array)
{
    QStringList values;
    for (const QJsonValue &value : array) {
        const QString text = value.toString().trimmed();
        if (!text.isEmpty() && !values.contains(text, Qt::CaseInsensitive))
            values.append(text);
    }
    values.sort(Qt::CaseInsensitive);
    return values;
}

QJsonArray stringListToJson(const QStringList &values)
{
    QJsonArray array;
    QStringList cleaned;
    for (const QString &value : values) {
        const QString text = value.trimmed();
        if (!text.isEmpty() && !cleaned.contains(text, Qt::CaseInsensitive))
            cleaned.append(text);
    }
    cleaned.sort(Qt::CaseInsensitive);
    for (const QString &value : cleaned)
        array.append(value);
    return array;
}

QString htmlToPlainText(const QString &html)
{
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText();
}

void sortNotes(QList<Note> &notes)
{
    std::sort(notes.begin(), notes.end(), [](const Note &left, const Note &right) {
        if (left.pinned != right.pinned)
            return left.pinned;
        return left.updatedAt > right.updatedAt;
    });
}

bool noteMatchesQuery(const Note &note, const QString &needle)
{
    if (needle.isEmpty())
        return true;

    if (note.title.contains(needle, Qt::CaseInsensitive)
        || note.body.contains(needle, Qt::CaseInsensitive)
        || note.html.contains(needle, Qt::CaseInsensitive)
        || note.tags.join(QLatin1Char(' ')).contains(needle, Qt::CaseInsensitive)) {
        return true;
    }

    for (const NoteAttachment &attachment : note.attachments) {
        if (attachment.displayName.contains(needle, Qt::CaseInsensitive)
            || attachment.filePath.contains(needle, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

bool noteHasAnyTag(const Note &note, const QStringList &tags)
{
    for (const QString &tag : tags) {
        if (note.tags.contains(tag, Qt::CaseInsensitive))
            return true;
    }
    return tags.isEmpty();
}

bool noteHasAllTags(const Note &note, const QStringList &tags)
{
    for (const QString &tag : tags) {
        if (!note.tags.contains(tag, Qt::CaseInsensitive))
            return false;
    }
    return true;
}
}

QJsonObject NoteAttachment::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), id);
    object.insert(QStringLiteral("filePath"), filePath);
    object.insert(QStringLiteral("displayName"), displayName);
    object.insert(QStringLiteral("mimeType"), mimeType);
    object.insert(QStringLiteral("size"), static_cast<double>(size));
    object.insert(QStringLiteral("addedAt"), isoString(addedAt));
    return object;
}

NoteAttachment NoteAttachment::fromJson(const QJsonObject &object)
{
    NoteAttachment attachment;
    attachment.id = object.value(QStringLiteral("id")).toString();
    attachment.filePath = object.value(QStringLiteral("filePath")).toString();
    attachment.displayName = object.value(QStringLiteral("displayName")).toString();
    attachment.mimeType = object.value(QStringLiteral("mimeType")).toString();
    attachment.size = static_cast<qint64>(object.value(QStringLiteral("size")).toDouble());
    attachment.addedAt = fromIsoString(object.value(QStringLiteral("addedAt")).toString());
    return attachment;
}

QJsonObject Note::toJson() const
{
    QJsonArray attachmentsArray;
    for (const NoteAttachment &attachment : attachments)
        attachmentsArray.append(attachment.toJson());

    QJsonObject object;
    object.insert(QStringLiteral("id"), id);
    object.insert(QStringLiteral("title"), title);
    object.insert(QStringLiteral("body"), body);
    object.insert(QStringLiteral("html"), html);
    object.insert(QStringLiteral("folderId"), folderId);
    object.insert(QStringLiteral("tags"), stringListToJson(tags));
    object.insert(QStringLiteral("attachments"), attachmentsArray);
    object.insert(QStringLiteral("pinned"), pinned);
    object.insert(QStringLiteral("trashed"), trashed);
    object.insert(QStringLiteral("locked"), locked);
    object.insert(QStringLiteral("passwordHash"), passwordHash);
    object.insert(QStringLiteral("passwordHint"), passwordHint);
    object.insert(QStringLiteral("createdAt"), isoString(createdAt));
    object.insert(QStringLiteral("updatedAt"), isoString(updatedAt));
    object.insert(QStringLiteral("trashedAt"), isoString(trashedAt));
    object.insert(QStringLiteral("driveFileId"), driveFileId);
    object.insert(QStringLiteral("lastSyncedAt"), isoString(lastSyncedAt));
    return object;
}

Note Note::fromJson(const QJsonObject &object)
{
    Note note;
    note.id = object.value(QStringLiteral("id")).toString();
    note.title = object.value(QStringLiteral("title")).toString(QStringLiteral("Untitled Note"));
    note.body = object.value(QStringLiteral("body")).toString();
    note.html = object.value(QStringLiteral("html")).toString();
    note.folderId = object.value(QStringLiteral("folderId")).toString();
    note.tags = jsonToStringList(object.value(QStringLiteral("tags")).toArray());
    note.pinned = object.value(QStringLiteral("pinned")).toBool();
    note.trashed = object.value(QStringLiteral("trashed")).toBool();
    note.locked = object.value(QStringLiteral("locked")).toBool();
    note.passwordHash = object.value(QStringLiteral("passwordHash")).toString();
    note.passwordHint = object.value(QStringLiteral("passwordHint")).toString();
    note.createdAt = fromIsoString(object.value(QStringLiteral("createdAt")).toString());
    note.updatedAt = fromIsoString(object.value(QStringLiteral("updatedAt")).toString());
    note.trashedAt = fromIsoString(object.value(QStringLiteral("trashedAt")).toString());
    note.driveFileId = object.value(QStringLiteral("driveFileId")).toString();
    note.lastSyncedAt = fromIsoString(object.value(QStringLiteral("lastSyncedAt")).toString());

    const QJsonArray attachmentsArray = object.value(QStringLiteral("attachments")).toArray();
    note.attachments.reserve(attachmentsArray.size());
    for (const QJsonValue &value : attachmentsArray)
        note.attachments.append(NoteAttachment::fromJson(value.toObject()));

    return note;
}

QJsonObject NoteFolder::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), id);
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("parentId"), parentId);
    object.insert(QStringLiteral("createdAt"), isoString(createdAt));
    return object;
}

NoteFolder NoteFolder::fromJson(const QJsonObject &object)
{
    NoteFolder folder;
    folder.id = object.value(QStringLiteral("id")).toString();
    folder.name = object.value(QStringLiteral("name")).toString(QStringLiteral("New Folder"));
    folder.parentId = object.value(QStringLiteral("parentId")).toString();
    folder.createdAt = fromIsoString(object.value(QStringLiteral("createdAt")).toString());
    return folder;
}

QJsonObject SmartFolder::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), id);
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("builtInKind"), builtInKind);
    object.insert(QStringLiteral("requiredTags"), stringListToJson(requiredTags));
    object.insert(QStringLiteral("matchAllTags"), matchAllTags);
    object.insert(QStringLiteral("requirePinned"), requirePinned);
    object.insert(QStringLiteral("requireAttachments"), requireAttachments);
    object.insert(QStringLiteral("requireLocked"), requireLocked);
    object.insert(QStringLiteral("createdAt"), isoString(createdAt));
    return object;
}

SmartFolder SmartFolder::fromJson(const QJsonObject &object)
{
    SmartFolder folder;
    folder.id = object.value(QStringLiteral("id")).toString();
    folder.name = object.value(QStringLiteral("name")).toString(QStringLiteral("Smart Folder"));
    folder.builtInKind = object.value(QStringLiteral("builtInKind")).toString();
    folder.requiredTags = jsonToStringList(object.value(QStringLiteral("requiredTags")).toArray());
    folder.matchAllTags = object.value(QStringLiteral("matchAllTags")).toBool();
    folder.requirePinned = object.value(QStringLiteral("requirePinned")).toBool();
    folder.requireAttachments = object.value(QStringLiteral("requireAttachments")).toBool();
    folder.requireLocked = object.value(QStringLiteral("requireLocked")).toBool();
    folder.createdAt = fromIsoString(object.value(QStringLiteral("createdAt")).toString());
    return folder;
}

NotesManager::NotesManager(QObject *parent)
    : QObject(parent)
    , m_storageDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
    , m_autoSaveTimer(new QTimer(this))
{
    m_autoSaveTimer->setSingleShot(true);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &NotesManager::flushPendingChanges);
}

NotesManager::~NotesManager()
{
    if (m_dirty)
        save();
}

QString NotesManager::storageDirectory() const
{
    return m_storageDir;
}

void NotesManager::setStorageDirectory(const QString &directoryPath)
{
    if (!directoryPath.isEmpty() && directoryPath != m_storageDir)
        m_storageDir = directoryPath;
}

QString NotesManager::libraryFilePath() const
{
    return NoteStorage(m_storageDir).libraryFilePath();
}

QString NotesManager::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool NotesManager::load()
{
    NotesLibraryData data;
    if (!NoteStorage(m_storageDir).load(&data))
        return false;

    m_notes = data.notes;
    for (Note &note : m_notes)
        normalizeNote(note);

    m_folders = data.folders;
    for (NoteFolder &folder : m_folders) {
        if (folder.id.isEmpty())
            folder.id = generateId();
        folder.name = FolderManager::normalizedName(folder.name);
        if (indexOfFolder(folder.parentId) < 0)
            folder.parentId.clear();
    }

    m_smartFolders = data.smartFolders;
    for (SmartFolder &folder : m_smartFolders)
        folder.requiredTags = TagManager::normalizeTags(folder.requiredTags);

    m_recentNoteIds.clear();
    for (const QString &id : data.recentNoteIds) {
        if (!id.isEmpty() && indexOfNote(id) >= 0)
            m_recentNoteIds.append(id);
    }

    m_dirty = false;
    emit dataReloaded();
    return true;
}

bool NotesManager::save()
{
    NotesLibraryData data;
    data.notes = m_notes;
    data.folders = m_folders;
    data.smartFolders = m_smartFolders;
    data.recentNoteIds = m_recentNoteIds;

    if (!NoteStorage(m_storageDir).save(data))
        return false;

    m_dirty = false;
    return true;
}

QString NotesManager::createNote(const QString &title, const QString &body, const QString &folderId)
{
    if (!folderId.isEmpty() && indexOfFolder(folderId) < 0)
        return QString();

    Note note;
    note.id = generateId();
    note.title = title.trimmed().isEmpty() ? QStringLiteral("Untitled Note") : title.trimmed();
    note.body = body;
    note.folderId = folderId;
    note.createdAt = QDateTime::currentDateTimeUtc();
    note.updatedAt = note.createdAt;
    normalizeNote(note);

    m_notes.prepend(note);
    touchRecent(note.id);
    markDirty();

    emit noteCreated(note.id);
    return note.id;
}

QString NotesManager::duplicateNote(const QString &noteId)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return QString();

    Note copy = m_notes.at(idx);
    copy.id = generateId();
    copy.title = copy.title.trimmed().isEmpty()
                     ? QStringLiteral("Untitled Note Copy")
                     : QStringLiteral("%1 Copy").arg(copy.title);
    copy.pinned = false;
    copy.trashed = false;
    copy.trashedAt = QDateTime();
    copy.createdAt = QDateTime::currentDateTimeUtc();
    copy.updatedAt = copy.createdAt;
    copy.driveFileId.clear();
    copy.lastSyncedAt = QDateTime();
    normalizeNote(copy);

    m_notes.prepend(copy);
    touchRecent(copy.id);
    markDirty();
    emit noteCreated(copy.id);
    return copy.id;
}

bool NotesManager::renameNote(const QString &noteId, const QString &newTitle)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    Note &target = m_notes[idx];
    target.title = newTitle.trimmed().isEmpty() ? QStringLiteral("Untitled Note") : newTitle.trimmed();
    target.updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::updateNoteBody(const QString &noteId, const QString &body)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    Note &target = m_notes[idx];
    if (target.body == body)
        return true;

    target.body = body;
    target.html.clear();
    target.updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::updateNoteContent(const QString &noteId,
                                     const QString &title,
                                     const QString &body,
                                     const QString &html,
                                     const QStringList &tags)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    Note &target = m_notes[idx];
    const QString nextTitle = title.trimmed().isEmpty() ? QStringLiteral("Untitled Note") : title.trimmed();
    const QStringList nextTags = TagManager::normalizeTags(tags);

    if (target.title == nextTitle && target.body == body && target.html == html && target.tags == nextTags)
        return true;

    target.title = nextTitle;
    target.body = body;
    target.html = html;
    target.tags = nextTags;
    target.updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::setPinned(const QString &noteId, bool pinned)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    Note &target = m_notes[idx];
    if (target.pinned == pinned)
        return true;

    target.pinned = pinned;
    target.updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::moveNoteToFolder(const QString &noteId, const QString &folderId)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    if (!folderId.isEmpty() && indexOfFolder(folderId) < 0)
        return false;

    Note &target = m_notes[idx];
    if (target.folderId == folderId)
        return true;

    target.folderId = folderId;
    target.updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::setTags(const QString &noteId, const QStringList &tags)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    const QStringList cleaned = TagManager::normalizeTags(tags);
    if (m_notes[idx].tags == cleaned)
        return true;

    m_notes[idx].tags = cleaned;
    m_notes[idx].updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::addAttachment(const QString &noteId, const QString &filePath)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0 || filePath.trimmed().isEmpty())
        return false;

    QFileInfo info(filePath);
    NoteAttachment attachment;
    attachment.id = generateId();
    attachment.filePath = info.absoluteFilePath();
    attachment.displayName = info.fileName();
    attachment.size = info.exists() ? info.size() : 0;
    attachment.mimeType = QMimeDatabase().mimeTypeForFile(info).name();
    attachment.addedAt = QDateTime::currentDateTimeUtc();

    m_notes[idx].attachments.append(attachment);
    m_notes[idx].updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::removeAttachment(const QString &noteId, const QString &attachmentId)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    QList<NoteAttachment> &attachments = m_notes[idx].attachments;
    for (int i = 0; i < attachments.size(); ++i) {
        if (attachments.at(i).id == attachmentId) {
            attachments.removeAt(i);
            m_notes[idx].updatedAt = QDateTime::currentDateTimeUtc();
            markDirty();
            emit noteUpdated(noteId);
            return true;
        }
    }

    return false;
}

bool NotesManager::lockNote(const QString &noteId, const QString &password, const QString &hint)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0 || password.isEmpty())
        return false;

    Note &target = m_notes[idx];
    target.locked = true;
    target.passwordHash = NoteLockService::hashPassword(password);
    target.passwordHint = hint.trimmed();
    target.updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::unlockNote(const QString &noteId, const QString &password) const
{
    const int idx = indexOfNote(noteId);
    if (idx < 0 || password.isEmpty())
        return false;

    const Note &target = m_notes.at(idx);
    return target.locked && NoteLockService::passwordMatches(password, target.passwordHash);
}

bool NotesManager::removeLock(const QString &noteId, const QString &password)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    Note &target = m_notes[idx];
    if (!target.locked)
        return true;

    if (!NoteLockService::passwordMatches(password, target.passwordHash))
        return false;

    target.locked = false;
    target.passwordHash.clear();
    target.passwordHint.clear();
    target.updatedAt = QDateTime::currentDateTimeUtc();
    markDirty();
    emit noteUpdated(noteId);
    return true;
}

bool NotesManager::deleteNote(const QString &noteId)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    Note &target = m_notes[idx];
    target.trashed = true;
    target.trashedAt = QDateTime::currentDateTimeUtc();
    target.updatedAt = target.trashedAt;
    m_recentNoteIds.removeAll(noteId);
    markDirty();
    emit noteDeleted(noteId);
    return true;
}

bool NotesManager::restoreNote(const QString &noteId)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    Note &target = m_notes[idx];
    target.trashed = false;
    target.trashedAt = QDateTime();
    target.updatedAt = QDateTime::currentDateTimeUtc();
    touchRecent(noteId);
    markDirty();
    emit noteRestored(noteId);
    return true;
}

bool NotesManager::purgeNote(const QString &noteId)
{
    const int idx = indexOfNote(noteId);
    if (idx < 0)
        return false;

    m_notes.removeAt(idx);
    m_recentNoteIds.removeAll(noteId);
    markDirty();
    emit notePurged(noteId);
    return true;
}

void NotesManager::emptyTrash()
{
    QList<Note> survivors;
    for (const Note &note : m_notes) {
        if (note.trashed)
            emit notePurged(note.id);
        else
            survivors.append(note);
    }
    m_notes = survivors;
    markDirty();
}

bool NotesManager::hasNote(const QString &noteId) const
{
    return indexOfNote(noteId) >= 0;
}

Note NotesManager::note(const QString &noteId) const
{
    const int idx = indexOfNote(noteId);
    return idx >= 0 ? m_notes.at(idx) : Note();
}

QList<Note> NotesManager::allNotes() const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!note.trashed)
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::notesInFolder(const QString &folderId) const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!note.trashed && note.folderId == folderId)
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::pinnedNotes() const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!note.trashed && note.pinned)
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::recentNotes(int limit) const
{
    QList<Note> result;
    if (limit <= 0)
        return result;

    for (const QString &id : m_recentNoteIds) {
        if (result.size() >= limit)
            break;
        const int idx = indexOfNote(id);
        if (idx >= 0 && !m_notes.at(idx).trashed)
            result.append(m_notes.at(idx));
    }
    return result;
}

QList<Note> NotesManager::attachmentNotes() const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!note.trashed && !note.attachments.isEmpty())
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::imageAttachmentNotes() const
{
    QList<Note> result = AttachmentManager::notesWithImages(allNotes());
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::pdfAttachmentNotes() const
{
    QList<Note> result = AttachmentManager::notesWithPdfs(allNotes());
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::fileAttachmentNotes() const
{
    QList<Note> result = AttachmentManager::notesWithGeneralFiles(allNotes());
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::createdTodayNotes() const
{
    QList<Note> result = SmartFolderManager::filterBuiltIn(allNotes(), SmartFolderManager::createdTodayKind());
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::modifiedTodayNotes() const
{
    QList<Note> result = SmartFolderManager::filterBuiltIn(allNotes(), SmartFolderManager::modifiedTodayKind());
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::modifiedThisWeekNotes() const
{
    QList<Note> result = SmartFolderManager::filterBuiltIn(allNotes(), SmartFolderManager::modifiedThisWeekKind());
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::untaggedNotes() const
{
    QList<Note> result = SmartFolderManager::filterBuiltIn(allNotes(), SmartFolderManager::untaggedKind());
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::lockedNotes() const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!note.trashed && note.locked)
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::trashedNotes() const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (note.trashed)
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::notesWithTag(const QString &tag) const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!note.trashed && note.tags.contains(tag, Qt::CaseInsensitive))
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::notesWithTags(const QStringList &tags, bool matchAll) const
{
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!note.trashed && TagManager::noteMatchesTags(note, tags, matchAll))
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::notesInSmartFolder(const QString &smartFolderId) const
{
    const SmartFolder folder = smartFolder(smartFolderId);
    if (folder.id.isEmpty())
        return {};

    QList<Note> result;
    for (const Note &note : m_notes) {
        if (SmartFolderManager::matchesCustomSmartFolder(note, folder))
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::notesForBuiltInSmartFolder(const QString &kind) const
{
    if (kind == SmartFolderManager::recentKind())
        return recentNotes(50);

    QList<Note> result = SmartFolderManager::filterBuiltIn(allNotes(), kind);
    sortNotes(result);
    return result;
}

QList<Note> NotesManager::searchNotes(const QString &query, bool includeTrashed) const
{
    const QString needle = query.trimmed();
    QList<Note> result;
    for (const Note &note : m_notes) {
        if (!includeTrashed && note.trashed)
            continue;
        if (noteMatchesQuery(note, needle))
            result.append(note);
    }
    sortNotes(result);
    return result;
}

QStringList NotesManager::allTags() const
{
    QSet<QString> seen;
    QStringList result;
    for (const Note &note : m_notes) {
        if (note.trashed)
            continue;
        for (const QString &tag : note.tags) {
            const QString cleaned = tag.trimmed();
            if (!cleaned.isEmpty() && !seen.contains(cleaned.toLower())) {
                seen.insert(cleaned.toLower());
                result.append(cleaned);
            }
        }
    }
    return TagManager::mergeWithDefaults(result);
}

void NotesManager::renameTag(const QString &oldTag, const QString &newTag)
{
    const QString oldClean = oldTag.trimmed();
    const QString newClean = newTag.trimmed();
    if (oldClean.isEmpty() || newClean.isEmpty())
        return;

    bool changed = false;
    for (Note &note : m_notes) {
        for (QString &tag : note.tags) {
            if (tag.compare(oldClean, Qt::CaseInsensitive) == 0) {
                tag = newClean;
                changed = true;
            }
        }
        note.tags = TagManager::normalizeTags(note.tags);
    }

    for (SmartFolder &folder : m_smartFolders) {
        for (QString &tag : folder.requiredTags) {
            if (tag.compare(oldClean, Qt::CaseInsensitive) == 0) {
                tag = newClean;
                changed = true;
            }
        }
        folder.requiredTags = TagManager::normalizeTags(folder.requiredTags);
    }

    if (changed) {
        markDirty();
        emit dataReloaded();
    }
}

void NotesManager::deleteTag(const QString &tag)
{
    const QString clean = tag.trimmed();
    if (clean.isEmpty())
        return;

    bool changed = false;
    for (Note &note : m_notes) {
        const int before = note.tags.size();
        note.tags.removeAll(clean);
        for (int i = note.tags.size() - 1; i >= 0; --i) {
            if (note.tags.at(i).compare(clean, Qt::CaseInsensitive) == 0)
                note.tags.removeAt(i);
        }
        changed = changed || before != note.tags.size();
    }

    for (SmartFolder &folder : m_smartFolders) {
        const int before = folder.requiredTags.size();
        for (int i = folder.requiredTags.size() - 1; i >= 0; --i) {
            if (folder.requiredTags.at(i).compare(clean, Qt::CaseInsensitive) == 0)
                folder.requiredTags.removeAt(i);
        }
        changed = changed || before != folder.requiredTags.size();
    }

    if (changed) {
        markDirty();
        emit dataReloaded();
    }
}

void NotesManager::touchRecent(const QString &noteId)
{
    if (noteId.isEmpty() || indexOfNote(noteId) < 0)
        return;

    m_recentNoteIds.removeAll(noteId);
    m_recentNoteIds.prepend(noteId);
    while (m_recentNoteIds.size() > kMaxRecentNotes)
        m_recentNoteIds.removeLast();
    markDirty();
}

QString NotesManager::createFolder(const QString &name, const QString &parentFolderId)
{
    if (!parentFolderId.isEmpty() && indexOfFolder(parentFolderId) < 0)
        return QString();

    NoteFolder folder;
    folder.id = generateId();
    folder.name = FolderManager::normalizedName(name);
    folder.parentId = parentFolderId;
    folder.createdAt = QDateTime::currentDateTimeUtc();

    m_folders.append(folder);
    markDirty();
    emit folderCreated(folder.id);
    return folder.id;
}

bool NotesManager::renameFolder(const QString &folderId, const QString &newName)
{
    const int idx = indexOfFolder(folderId);
    if (idx < 0)
        return false;

    m_folders[idx].name = FolderManager::normalizedName(newName);
    markDirty();
    emit folderRenamed(folderId);
    return true;
}

bool NotesManager::deleteFolder(const QString &folderId)
{
    const int idx = indexOfFolder(folderId);
    if (idx < 0)
        return false;

    QStringList deletedFolderIds = FolderManager::descendantFolderIds(m_folders, folderId);
    deletedFolderIds.prepend(folderId);

    for (Note &note : m_notes) {
        if (deletedFolderIds.contains(note.folderId)) {
            note.folderId.clear();
            note.updatedAt = QDateTime::currentDateTimeUtc();
        }
    }

    for (int i = m_folders.size() - 1; i >= 0; --i) {
        if (deletedFolderIds.contains(m_folders.at(i).id))
            m_folders.removeAt(i);
    }
    markDirty();
    emit folderDeleted(folderId);
    return true;
}

QList<NoteFolder> NotesManager::allFolders() const
{
    return FolderManager::sortedForDisplay(m_folders);
}

QString NotesManager::folderDisplayName(const QString &folderId) const
{
    return FolderManager::folderPath(m_folders, folderId);
}

QString NotesManager::createSmartFolder(const QString &name,
                                        const QStringList &requiredTags,
                                        bool matchAllTags,
                                        bool requirePinned,
                                        bool requireAttachments,
                                        bool requireLocked)
{
    SmartFolder folder;
    folder.id = generateId();
    folder.name = name.trimmed().isEmpty() ? QStringLiteral("Smart Folder") : name.trimmed();
    folder.requiredTags = TagManager::normalizeTags(requiredTags);
    folder.matchAllTags = matchAllTags;
    folder.requirePinned = requirePinned;
    folder.requireAttachments = requireAttachments;
    folder.requireLocked = requireLocked;
    folder.createdAt = QDateTime::currentDateTimeUtc();

    m_smartFolders.append(folder);
    markDirty();
    emit smartFolderCreated(folder.id);
    return folder.id;
}

bool NotesManager::updateSmartFolder(const SmartFolder &folder)
{
    const int idx = indexOfSmartFolder(folder.id);
    if (idx < 0)
        return false;

    m_smartFolders[idx] = folder;
    m_smartFolders[idx].requiredTags = TagManager::normalizeTags(folder.requiredTags);
    markDirty();
    emit smartFolderUpdated(folder.id);
    return true;
}

bool NotesManager::deleteSmartFolder(const QString &smartFolderId)
{
    const int idx = indexOfSmartFolder(smartFolderId);
    if (idx < 0)
        return false;

    m_smartFolders.removeAt(idx);
    markDirty();
    emit smartFolderDeleted(smartFolderId);
    return true;
}

QList<SmartFolder> NotesManager::allSmartFolders() const
{
    QList<SmartFolder> folders = m_smartFolders;
    std::sort(folders.begin(), folders.end(), [](const SmartFolder &left, const SmartFolder &right) {
        return left.name.compare(right.name, Qt::CaseInsensitive) < 0;
    });
    return folders;
}

SmartFolder NotesManager::smartFolder(const QString &smartFolderId) const
{
    const int idx = indexOfSmartFolder(smartFolderId);
    return idx >= 0 ? m_smartFolders.at(idx) : SmartFolder();
}

bool NotesManager::isAutoSaveEnabled() const
{
    return m_autoSaveEnabled;
}

void NotesManager::setAutoSaveEnabled(bool enabled)
{
    m_autoSaveEnabled = enabled;
    if (!enabled)
        m_autoSaveTimer->stop();
}

int NotesManager::autoSaveIntervalMs() const
{
    return m_autoSaveIntervalMs;
}

void NotesManager::setAutoSaveIntervalMs(int intervalMs)
{
    m_autoSaveIntervalMs = qMax(250, intervalMs);
}

bool NotesManager::hasUnsavedChanges() const
{
    return m_dirty;
}

void NotesManager::flushPendingChanges()
{
    if (!m_dirty)
        return;

    emit autoSaveStarted();
    const bool success = save();
    emit autoSaveFinished(success);

    if (success && m_syncEnabled)
        emit syncRequested();
}

bool NotesManager::isSyncEnabled() const
{
    return m_syncEnabled;
}

void NotesManager::setSyncEnabled(bool enabled)
{
    m_syncEnabled = enabled;
}

QString NotesManager::noteIdForTitle(const QString &title) const
{
    const QString clean = title.trimmed();
    if (clean.isEmpty())
        return QString();

    for (const Note &note : m_notes) {
        if (!note.trashed && note.title.compare(clean, Qt::CaseInsensitive) == 0)
            return note.id;
    }
    return QString();
}

int NotesManager::indexOfNote(const QString &noteId) const
{
    for (int i = 0; i < m_notes.size(); ++i) {
        if (m_notes.at(i).id == noteId)
            return i;
    }
    return -1;
}

int NotesManager::indexOfFolder(const QString &folderId) const
{
    for (int i = 0; i < m_folders.size(); ++i) {
        if (m_folders.at(i).id == folderId)
            return i;
    }
    return -1;
}

int NotesManager::indexOfSmartFolder(const QString &smartFolderId) const
{
    for (int i = 0; i < m_smartFolders.size(); ++i) {
        if (m_smartFolders.at(i).id == smartFolderId)
            return i;
    }
    return -1;
}

void NotesManager::normalizeNote(Note &note) const
{
    if (note.id.isEmpty())
        note.id = generateId();
    if (note.title.trimmed().isEmpty())
        note.title = QStringLiteral("Untitled Note");
    if (!note.html.isEmpty() && note.body.isEmpty())
        note.body = htmlToPlainText(note.html);
    if (note.createdAt.isNull())
        note.createdAt = QDateTime::currentDateTimeUtc();
    if (note.updatedAt.isNull())
        note.updatedAt = note.createdAt;
    note.tags = TagManager::normalizeTags(note.tags);
}

void NotesManager::markDirty()
{
    m_dirty = true;
    scheduleAutoSave();
}

void NotesManager::scheduleAutoSave()
{
    if (m_autoSaveEnabled)
        m_autoSaveTimer->start(m_autoSaveIntervalMs);
}
