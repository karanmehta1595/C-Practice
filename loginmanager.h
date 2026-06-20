// loginmanager.h
#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QObject>
#include <QString>

enum class LoginState {
    SignedOut,
    SigningIn,
    SignedIn,
    SignInError
};

class LoginManager : public QObject
{
    Q_OBJECT

public:
    explicit LoginManager(QObject *parent = nullptr);

    LoginState state() const;
    QString stateDescription() const;
    bool isSignedIn() const;

    QString displayName() const;
    QString email() const;
    QString avatarPath() const;

    void setUserProfile(const QString &displayName,
                        const QString &email,
                        const QString &avatarPath = QString());

    bool isDriveBackupEnabled() const;
    void setDriveBackupEnabled(bool enabled);

    bool isDriveSyncEnabled() const;
    void setDriveSyncEnabled(bool enabled);

public slots:
    void restoreSession();
    void beginGoogleSignIn();
    void signOut();
    void requestDriveBackup();
    void requestDriveSync();

signals:
    void loginStateChanged(LoginState state);
    void signedIn();
    void signedOut();
    void signInFailed(const QString &reason);
    void profileUpdated();
    void driveBackupEnabledChanged(bool enabled);
    void driveBackupRequested();
    void driveBackupCompleted(bool success);
    void driveSyncEnabledChanged(bool enabled);
    void driveSyncRequested();
    void driveSyncCompleted(bool success);

private:
    void setState(LoginState newState);
    void persistCachedProfile() const;

    LoginState m_state = LoginState::SignedOut;
    QString m_displayName;
    QString m_email;
    QString m_avatarPath;
    bool m_driveBackupEnabled = false;
    bool m_driveSyncEnabled = false;
};

#endif // LOGINMANAGER_H
