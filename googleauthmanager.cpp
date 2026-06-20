#include "googleauthmanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDateTime>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QBuffer>
#include <QDebug>

GoogleAuthManager::GoogleAuthManager(QObject *parent)
    : QObject(parent)
{
    m_net = new QNetworkAccessManager(this);

    // Restore cached profile info (so the About panel can show it
    // immediately on launch, before any token refresh round-trip).
    QSettings s("1OS", "Calculator");
    s.beginGroup(kSettingsGroup);
    m_name  = s.value("name").toString();
    m_email = s.value("email").toString();
    QByteArray picData = s.value("pictureData").toByteArray();
    if (!picData.isEmpty()) {
        QPixmap pm;
        pm.loadFromData(picData, "PNG");
        m_picture = pm;
    }
    s.endGroup();
}

bool GoogleAuthManager::loadClientConfig(const QString &clientSecretJsonPath)
{
    QFile f(clientSecretJsonPath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "GoogleAuthManager: could not open" << clientSecretJsonPath;
        return false;
    }
    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
    f.close();
    if (perr.error != QJsonParseError::NoError) {
        qWarning() << "GoogleAuthManager: JSON parse error" << perr.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    // Desktop OAuth client JSON is nested under "installed".
    QJsonObject installed = root.value("installed").toObject();
    if (installed.isEmpty()) installed = root.value("web").toObject();
    if (installed.isEmpty()) {
        qWarning() << "GoogleAuthManager: unrecognised client_secret.json shape";
        return false;
    }

    m_clientId     = installed.value("client_id").toString();
    m_clientSecret = installed.value("client_secret").toString();

    return !m_clientId.isEmpty();
}

bool GoogleAuthManager::isSignedIn() const
{
    QSettings s("1OS", "Calculator");
    s.beginGroup(kSettingsGroup);
    bool has = !s.value("refreshToken").toString().isEmpty();
    s.endGroup();
    return has;
}

// ── PKCE helpers ──────────────────────────────────────────────────

QString GoogleAuthManager::generateRandomString(int length)
{
    static const QString alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString out;
    out.reserve(length);
    for (int i = 0; i < length; i++)
        out.append(alphabet.at(QRandomGenerator::global()->bounded(alphabet.size())));
    return out;
}

QString GoogleAuthManager::pkceChallengeFromVerifier(const QString &verifier)
{
    QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromUtf8(hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// ── Sign-in flow ──────────────────────────────────────────────────

void GoogleAuthManager::signIn()
{
    if (m_clientId.isEmpty()) {
        emit signInFailed("OAuth client not configured (client_secret.json missing/invalid).");
        return;
    }
    startLoopbackServer();
}

void GoogleAuthManager::startLoopbackServer()
{
    if (m_loopback) {
        m_loopback->close();
        m_loopback->deleteLater();
        m_loopback = nullptr;
    }

    m_loopback = new QTcpServer(this);
    if (!m_loopback->listen(QHostAddress::LocalHost, 0)) {
        emit signInFailed("Could not start local redirect listener: " + m_loopback->errorString());
        return;
    }

    quint16 port = m_loopback->serverPort();
    const QString redirectUri = QString("http://localhost:%1/").arg(port);

    m_codeVerifier = generateRandomString(64);
    const QString challenge = pkceChallengeFromVerifier(m_codeVerifier);

    QUrl authUrl(kAuthEndpoint);
    QUrlQuery q;
    q.addQueryItem("client_id", m_clientId);
    q.addQueryItem("redirect_uri", redirectUri);
    q.addQueryItem("response_type", "code");
    q.addQueryItem("scope", kScopes);
    q.addQueryItem("code_challenge", challenge);
    q.addQueryItem("code_challenge_method", "S256");
    q.addQueryItem("access_type", "offline");
    q.addQueryItem("prompt", "consent select_account");
    authUrl.setQuery(q);

    connect(m_loopback, &QTcpServer::newConnection, this, [this, redirectUri]() {
        QTcpSocket *sock = m_loopback->nextPendingConnection();
        if (!sock) return;

        connect(sock, &QTcpSocket::readyRead, this, [this, sock, redirectUri]() {
            QByteArray data = sock->readAll();
            // Parse the first request line: "GET /?code=...&scope=... HTTP/1.1"
            QString reqLine = QString::fromUtf8(data).split("\r\n").value(0);
            QString path = reqLine.section(' ', 1, 1);

            QUrl asUrl(QString("http://localhost") + path);
            QUrlQuery query(asUrl);

            const QString respBody =
                "<html><body style='font-family:sans-serif;text-align:center;padding-top:60px;'>"
                "<h2>1OS Calculator</h2><p>You can close this tab and return to the app.</p>"
                "</body></html>";
            QByteArray httpResp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
                + QByteArray::number(respBody.size()) + "\r\nConnection: close\r\n\r\n" + respBody.toUtf8();
            sock->write(httpResp);
            sock->flush();
            sock->disconnectFromHost();

            if (query.hasQueryItem("error")) {
                emit signInFailed("Google denied access: " + query.queryItemValue("error"));
                m_loopback->close();
                return;
            }
            if (query.hasQueryItem("code")) {
                const QString code = query.queryItemValue("code");
                m_loopback->close();
                handleAuthCode(code, redirectUri);
            }
        });

        connect(sock, &QTcpSocket::disconnected, sock, &QTcpSocket::deleteLater);
    });

    QDesktopServices::openUrl(authUrl);
}

void GoogleAuthManager::handleAuthCode(const QString &code, const QString &redirectUri)
{
    exchangeCodeForTokens(code, redirectUri);
}

void GoogleAuthManager::exchangeCodeForTokens(const QString &code, const QString &redirectUri)
{
    QUrlQuery body;
    body.addQueryItem("code", code);
    body.addQueryItem("client_id", m_clientId);
    if (!m_clientSecret.isEmpty())
        body.addQueryItem("client_secret", m_clientSecret);
    body.addQueryItem("redirect_uri", redirectUri);
    body.addQueryItem("grant_type", "authorization_code");
    body.addQueryItem("code_verifier", m_codeVerifier);

    QNetworkRequest req{QUrl(kTokenEndpoint)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_net->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit signInFailed("Token exchange failed: " + reply->errorString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString accessToken  = obj.value("access_token").toString();
        const QString refreshToken = obj.value("refresh_token").toString();
        const int expiresIn        = obj.value("expires_in").toInt(3600);

        if (accessToken.isEmpty()) {
            emit signInFailed("Token exchange returned no access_token.");
            return;
        }
        persistTokens(accessToken, refreshToken, expiresIn);
        fetchUserProfile();
    });
}

void GoogleAuthManager::persistTokens(const QString &accessToken, const QString &refreshToken, int expiresInSecs)
{
    QSettings s("1OS", "Calculator");
    s.beginGroup(kSettingsGroup);
    s.setValue("accessToken", accessToken);
    if (!refreshToken.isEmpty())
        s.setValue("refreshToken", refreshToken); // refresh_token is only sent on first consent
    s.setValue("accessTokenExpiry", QDateTime::currentDateTimeUtc().addSecs(expiresInSecs));
    s.endGroup();
}

void GoogleAuthManager::ensureValidAccessToken(std::function<void(const QString &)> callback)
{
    QSettings s("1OS", "Calculator");
    s.beginGroup(kSettingsGroup);
    QString accessToken = s.value("accessToken").toString();
    QDateTime expiry     = s.value("accessTokenExpiry").toDateTime();
    QString refreshToken = s.value("refreshToken").toString();
    s.endGroup();

    if (refreshToken.isEmpty()) {
        callback(QString());
        return;
    }

    // Refresh proactively if expired or expiring within 60s.
    if (!accessToken.isEmpty() && expiry.isValid() &&
        QDateTime::currentDateTimeUtc().addSecs(60) < expiry) {
        callback(accessToken);
        return;
    }

    refreshAccessToken(callback);
}

void GoogleAuthManager::refreshAccessToken(std::function<void(const QString &)> callback)
{
    QSettings s("1OS", "Calculator");
    s.beginGroup(kSettingsGroup);
    QString refreshToken = s.value("refreshToken").toString();
    s.endGroup();

    if (refreshToken.isEmpty()) {
        callback(QString());
        return;
    }

    QUrlQuery body;
    body.addQueryItem("client_id", m_clientId);
    if (!m_clientSecret.isEmpty())
        body.addQueryItem("client_secret", m_clientSecret);
    body.addQueryItem("refresh_token", refreshToken);
    body.addQueryItem("grant_type", "refresh_token");

    QNetworkRequest req{QUrl(kTokenEndpoint)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_net->post(req, body.query(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "GoogleAuthManager: refresh failed" << reply->errorString();
            callback(QString());
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString accessToken = obj.value("access_token").toString();
        const int expiresIn = obj.value("expires_in").toInt(3600);
        if (accessToken.isEmpty()) {
            callback(QString());
            return;
        }
        persistTokens(accessToken, QString(), expiresIn); // keep existing refresh token
        callback(accessToken);
    });
}

void GoogleAuthManager::fetchUserProfile()
{
    ensureValidAccessToken([this](const QString &accessToken) {
        if (accessToken.isEmpty()) {
            emit signInFailed("Could not obtain access token for profile fetch.");
            return;
        }
        QNetworkRequest req{QUrl(kUserInfoEndpoint)};
        req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

        QNetworkReply *reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                emit signInFailed("Profile fetch failed: " + reply->errorString());
                return;
            }
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            m_name  = obj.value("name").toString();
            m_email = obj.value("email").toString();
            const QString pictureUrl = obj.value("picture").toString();

            QSettings s("1OS", "Calculator");
            s.beginGroup(kSettingsGroup);
            s.setValue("name", m_name);
            s.setValue("email", m_email);
            s.endGroup();

            emit signedIn();
            emit profileUpdated();

            if (!pictureUrl.isEmpty())
                downloadProfilePicture(pictureUrl);
        });
    });
}

void GoogleAuthManager::downloadProfilePicture(const QString &url)
{
    QNetworkReply *reply = m_net->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        QByteArray data = reply->readAll();
        QPixmap pm;
        if (pm.loadFromData(data)) {
            m_picture = pm;
            QSettings s("1OS", "Calculator");
            s.beginGroup(kSettingsGroup);
            QByteArray pngData;
            QBuffer buf(&pngData);
            buf.open(QIODevice::WriteOnly);
            pm.save(&buf, "PNG");
            s.setValue("pictureData", pngData);
            s.endGroup();
            emit profileUpdated();
        }
    });
}

void GoogleAuthManager::signOut()
{
    QSettings s("1OS", "Calculator");
    s.beginGroup(kSettingsGroup);
    s.remove(""); // clears entire group
    s.endGroup();
    m_name.clear();
    m_email.clear();
    m_picture = QPixmap();
    emit signedOut();
}
