#ifndef GOOGLEDRIVEBACKUP_H
#define GOOGLEDRIVEBACKUP_H

// ─────────────────────────────────────────────────────────────────
//  GoogleDriveBackup
//
//  Uploads/downloads a single JSON backup file to the user's
//  Google Drive "appDataFolder" — a special hidden space (scope
//  drive.appdata) that does NOT appear in the user's normal Drive
//  UI and is only accessible to the app that created it.
//
//  File is located by name within appDataFolder on each call
//  (Drive allows duplicate names, so we always look up the most
//  recent match and overwrite it rather than create new copies).
// ─────────────────────────────────────────────────────────────────

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <functional>

class GoogleAuthManager;

class GoogleDriveBackup : public QObject
{
    Q_OBJECT
public:
    explicit GoogleDriveBackup(GoogleAuthManager *authManager, QObject *parent = nullptr);

    // Uploads `payload` (already-serialized JSON object) as the backup file.
    // onDone receives (success, errorMessage).
    void backupNow(const QJsonObject &payload, std::function<void(bool, QString)> onDone);

    // Downloads and parses the backup file.
    // onDone receives (success, payload, errorMessage).
    void restoreNow(std::function<void(bool, QJsonObject, QString)> onDone);

    // Convenience: returns ISO timestamp string of the last successful
    // backup/restore (read from QSettings), or empty if none yet.
    static QString lastBackupTime();
    static QString lastRestoreTime();

private:
    void findExistingFileId(std::function<void(const QString &fileId)> onFound);
    void uploadMultipart(const QString &existingFileId, const QByteArray &jsonBytes,
                         std::function<void(bool, QString)> onDone);
    void downloadFileContent(const QString &fileId, std::function<void(bool, QByteArray, QString)> onDone);

    GoogleAuthManager *m_auth = nullptr;
    QNetworkAccessManager *m_net = nullptr;

    static constexpr const char *kBackupFileName = "1OS_Calculator_Backup.json";
    static constexpr const char *kFilesEndpoint  = "https://www.googleapis.com/drive/v3/files";
    static constexpr const char *kUploadEndpoint = "https://www.googleapis.com/upload/drive/v3/files";
};

#endif // GOOGLEDRIVEBACKUP_H
