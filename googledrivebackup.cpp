#include "googledrivebackup.h"
#include "googleauthmanager.h"

#include <QSettings>
#include <QDateTime>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDebug>

GoogleDriveBackup::GoogleDriveBackup(GoogleAuthManager *authManager, QObject *parent)
    : QObject(parent), m_auth(authManager)
{
    m_net = new QNetworkAccessManager(this);
}

QString GoogleDriveBackup::lastBackupTime()
{
    QSettings s("1OS", "Calculator");
    return s.value("GoogleDriveBackup/lastBackup").toString();
}

QString GoogleDriveBackup::lastRestoreTime()
{
    QSettings s("1OS", "Calculator");
    return s.value("GoogleDriveBackup/lastRestore").toString();
}

void GoogleDriveBackup::findExistingFileId(std::function<void(const QString &)> onFound)
{
    m_auth->ensureValidAccessToken([this, onFound](const QString &accessToken) {
        if (accessToken.isEmpty()) {
            onFound(QString());
            return;
        }

        QUrl url(kFilesEndpoint);
        QUrlQuery q;
        q.addQueryItem("spaces", "appDataFolder");
        q.addQueryItem("q", QString("name = '%1'").arg(kBackupFileName));
        q.addQueryItem("fields", "files(id,name,modifiedTime)");
        q.addQueryItem("pageSize", "10");
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

        QNetworkReply *reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [reply, onFound]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                onFound(QString());
                return;
            }
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            QJsonArray files = obj.value("files").toArray();
            if (files.isEmpty()) {
                onFound(QString());
                return;
            }
            // Files are not guaranteed sorted; pick most recently modified.
            QString bestId;
            QDateTime bestTime;
            for (const auto &v : files) {
                QJsonObject f = v.toObject();
                QDateTime t = QDateTime::fromString(f.value("modifiedTime").toString(), Qt::ISODate);
                if (bestId.isEmpty() || t > bestTime) {
                    bestId = f.value("id").toString();
                    bestTime = t;
                }
            }
            onFound(bestId);
        });
    });
}

void GoogleDriveBackup::backupNow(const QJsonObject &payload, std::function<void(bool, QString)> onDone)
{
    QByteArray jsonBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    findExistingFileId([this, jsonBytes, onDone](const QString &existingFileId) {
        uploadMultipart(existingFileId, jsonBytes, onDone);
    });
}

void GoogleDriveBackup::uploadMultipart(const QString &existingFileId, const QByteArray &jsonBytes,
                                         std::function<void(bool, QString)> onDone)
{
    m_auth->ensureValidAccessToken([this, existingFileId, jsonBytes, onDone](const QString &accessToken) {
        if (accessToken.isEmpty()) {
            onDone(false, "Not signed in / token unavailable.");
            return;
        }

        QJsonObject metadata;
        metadata["name"] = kBackupFileName;
        if (existingFileId.isEmpty()) {
            // Only set parents on create; Drive forbids changing
            // parents to appDataFolder on update via this field.
            metadata["parents"] = QJsonArray{ QString("appDataFolder") };
        }

        auto *multiPart = new QHttpMultiPart(QHttpMultiPart::RelatedType);

        QHttpPart metaPart;
        metaPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
        metaPart.setBody(QJsonDocument(metadata).toJson(QJsonDocument::Compact));
        multiPart->append(metaPart);

        QHttpPart dataPart;
        dataPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        dataPart.setBody(jsonBytes);
        multiPart->append(dataPart);

        QUrl url(kUploadEndpoint);
        QUrlQuery q;
        q.addQueryItem("uploadType", "multipart");
        url.setQuery(q);
        if (!existingFileId.isEmpty())
            url.setPath(url.path() + "/" + existingFileId);

        QNetworkRequest req(url);
        req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

        QNetworkReply *reply = nullptr;
        if (existingFileId.isEmpty()) {
            reply = m_net->post(req, multiPart);
        } else {
            reply = m_net->sendCustomRequest(req, "PATCH", multiPart);
        }
        multiPart->setParent(reply);

        connect(reply, &QNetworkReply::finished, this, [reply, onDone]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                onDone(false, "Upload failed: " + reply->errorString() + " — " + QString::fromUtf8(reply->readAll()));
                return;
            }
            QSettings s("1OS", "Calculator");
            s.setValue("GoogleDriveBackup/lastBackup", QDateTime::currentDateTime().toString(Qt::ISODate));
            onDone(true, QString());
        });
    });
}

void GoogleDriveBackup::downloadFileContent(const QString &fileId, std::function<void(bool, QByteArray, QString)> onDone)
{
    m_auth->ensureValidAccessToken([this, fileId, onDone](const QString &accessToken) {
        if (accessToken.isEmpty()) {
            onDone(false, QByteArray(), "Not signed in / token unavailable.");
            return;
        }
        QUrl url(QString("%1/%2").arg(kFilesEndpoint, fileId));
        QUrlQuery q;
        q.addQueryItem("alt", "media");
        url.setQuery(q);

        QNetworkRequest req(url);
        req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

        QNetworkReply *reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [reply, onDone]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                onDone(false, QByteArray(), "Download failed: " + reply->errorString());
                return;
            }
            onDone(true, reply->readAll(), QString());
        });
    });
}

void GoogleDriveBackup::restoreNow(std::function<void(bool, QJsonObject, QString)> onDone)
{
    findExistingFileId([this, onDone](const QString &fileId) {
        if (fileId.isEmpty()) {
            onDone(false, QJsonObject(), "No backup found in Google Drive.");
            return;
        }
        downloadFileContent(fileId, [onDone](bool ok, QByteArray data, QString err) {
            if (!ok) {
                onDone(false, QJsonObject(), err);
                return;
            }
            QJsonParseError perr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &perr);
            if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
                onDone(false, QJsonObject(), "Backup file was corrupted or unreadable.");
                return;
            }
            QSettings s("1OS", "Calculator");
            s.setValue("GoogleDriveBackup/lastRestore", QDateTime::currentDateTime().toString(Qt::ISODate));
            onDone(true, doc.object(), QString());
        });
    });
}
