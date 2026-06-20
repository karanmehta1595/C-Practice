#ifndef GOOGLEAUTHMANAGER_H
#define GOOGLEAUTHMANAGER_H

// ─────────────────────────────────────────────────────────────────
//  GoogleAuthManager
//
//  Handles the full OAuth 2.0 "Installed App" flow for Google
//  Sign-In using Authorization Code + PKCE:
//
//    1. Generates a PKCE code_verifier / code_challenge pair.
//    2. Starts a tiny local QTcpServer on 127.0.0.1 (loopback),
//       matching the "http://localhost" redirect URI registered
//       in the Desktop OAuth Client (client_secret.json).
//    3. Opens the system browser to Google's consent screen.
//    4. Catches the redirect on the loopback server, extracts the
//       ?code=... parameter, and closes the browser tab.
//    5. Exchanges the code for access_token + refresh_token.
//    6. Persists tokens in QSettings (per user's request) and
//       fetches the user's profile (name / email / picture) via
//       the Userinfo endpoint.
//    7. Silently refreshes the access_token using the refresh_token
//       when it expires, without requiring the browser again.
//
//  SECURITY NOTE: QSettings is plain-text storage (registry on
//  Windows, plist on macOS, ini file on Linux). It is NOT secure
//  storage. The user explicitly requested QSettings; for stronger
//  protection at-rest, consider QtKeychain or OS-native credential
//  vaults in the future.
// ─────────────────────────────────────────────────────────────────

#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QPixmap>
#include <functional>

class GoogleAuthManager : public QObject
{
    Q_OBJECT
public:
    explicit GoogleAuthManager(QObject *parent = nullptr);

    // Loads client_id/client_secret from the bundled OAuth client JSON.
    // Returns false if the file could not be parsed.
    bool loadClientConfig(const QString &clientSecretJsonPath);

    // True if we currently hold a (possibly expired) refresh token.
    bool isSignedIn() const;

    // Kicks off the full sign-in flow (browser + loopback listener).
    void signIn();

    // Clears all stored tokens/profile info from QSettings.
    void signOut();

    // Ensures we have a valid (non-expired) access token, refreshing
    // it first if necessary. Calls back with the token, or an empty
    // string on failure.
    void ensureValidAccessToken(std::function<void(const QString &accessToken)> callback);

    // Cached profile info (valid only after signedIn() is emitted).
    QString userName() const     { return m_name; }
    QString userEmail() const    { return m_email; }
    QPixmap userPicture() const  { return m_picture; }
    bool hasPicture() const      { return !m_picture.isNull(); }

signals:
    void signedIn();                       // sign-in + profile fetch succeeded
    void signInFailed(const QString &err); // sign-in failed at any step
    void signedOut();
    void profileUpdated();                 // picture/name/email refreshed

private:
    void startLoopbackServer();
    void handleAuthCode(const QString &code, const QString &redirectUri);
    void exchangeCodeForTokens(const QString &code, const QString &redirectUri);
    void refreshAccessToken(std::function<void(const QString &accessToken)> callback);
    void fetchUserProfile();
    void downloadProfilePicture(const QString &url);
    void persistTokens(const QString &accessToken, const QString &refreshToken, int expiresInSecs);
    static QString generateRandomString(int length);
    static QString pkceChallengeFromVerifier(const QString &verifier);

    QNetworkAccessManager *m_net = nullptr;
    QTcpServer *m_loopback = nullptr;

    QString m_clientId;
    QString m_clientSecret;

    QString m_codeVerifier;
    QString m_name;
    QString m_email;
    QPixmap m_picture;

    static constexpr const char *kSettingsGroup   = "GoogleAuth";
    static constexpr const char *kAuthEndpoint    = "https://accounts.google.com/o/oauth2/v2/auth";
    static constexpr const char *kTokenEndpoint   = "https://oauth2.googleapis.com/token";
    static constexpr const char *kUserInfoEndpoint= "https://www.googleapis.com/oauth2/v3/userinfo";
    static constexpr const char *kScopes          = "openid email profile https://www.googleapis.com/auth/drive.appdata";
};

#endif // GOOGLEAUTHMANAGER_H
