// loginmanager.cpp
#include "loginmanager.h"

#include <QSettings>

LoginManager::LoginManager(QObject *parent)
    : QObject(parent)
{
}

LoginState LoginManager::state() const
{
    return m_state;
}

QString LoginManager::stateDescription() const
{
    switch (m_state) {
    case LoginState::SignedOut:
        return tr("Not signed in");
    case LoginState::SigningIn:
        return tr("Signing in...");
    case LoginState::SignedIn:
        return m_displayName.isEmpty() ? tr("Signed in") : tr("Signed in as %1").arg(m_displayName);
    case LoginState::SignInError:
        return tr("Sign-in failed");
    }
    return QString();
}

bool LoginManager::isSignedIn() const
{
    return m_state == LoginState::SignedIn;
}

QString LoginManager::displayName() const
{
    return m_displayName;
}

QString LoginManager::email() const
{
    return m_email;
}

QString LoginManager::avatarPath() const
{
    return m_avatarPath;
}

void LoginManager::setUserProfile(const QString &displayName,
                                  const QString &email,
                                  const QString &avatarPath)
{
    m_displayName = displayName;
    m_email = email;
    m_avatarPath = avatarPath;
    persistCachedProfile();
    emit profileUpdated();
}

bool LoginManager::isDriveBackupEnabled() const
{
    return m_driveBackupEnabled;
}

void LoginManager::setDriveBackupEnabled(bool enabled)
{
    if (m_driveBackupEnabled == enabled)
        return;
    m_driveBackupEnabled = enabled;
    emit driveBackupEnabledChanged(enabled);
}

bool LoginManager::isDriveSyncEnabled() const
{
    return m_driveSyncEnabled;
}

void LoginManager::setDriveSyncEnabled(bool enabled)
{
    if (m_driveSyncEnabled == enabled)
        return;
    m_driveSyncEnabled = enabled;
    emit driveSyncEnabledChanged(enabled);
}

void LoginManager::restoreSession()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Account"));
    m_displayName = settings.value(QStringLiteral("DisplayName")).toString();
    m_email = settings.value(QStringLiteral("Email")).toString();
    m_avatarPath = settings.value(QStringLiteral("AvatarPath")).toString();
    settings.endGroup();

    emit profileUpdated();
    setState(LoginState::SignedOut);
}

void LoginManager::beginGoogleSignIn()
{
    setState(LoginState::SigningIn);
    const QString reason = tr("Google sign-in is not enabled in this local build yet.");
    setState(LoginState::SignInError);
    emit signInFailed(reason);
}

void LoginManager::signOut()
{
    m_displayName.clear();
    m_email.clear();
    m_avatarPath.clear();
    m_driveBackupEnabled = false;
    m_driveSyncEnabled = false;

    QSettings settings;
    settings.beginGroup(QStringLiteral("Account"));
    settings.remove(QString());
    settings.endGroup();

    emit profileUpdated();
    setState(LoginState::SignedOut);
    emit signedOut();
}

void LoginManager::requestDriveBackup()
{
    emit driveBackupRequested();
    emit driveBackupCompleted(false);
}

void LoginManager::requestDriveSync()
{
    emit driveSyncRequested();
    emit driveSyncCompleted(false);
}

void LoginManager::setState(LoginState newState)
{
    if (m_state == newState)
        return;

    m_state = newState;
    emit loginStateChanged(m_state);
    if (m_state == LoginState::SignedIn)
        emit signedIn();
}

void LoginManager::persistCachedProfile() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("Account"));
    settings.setValue(QStringLiteral("DisplayName"), m_displayName);
    settings.setValue(QStringLiteral("Email"), m_email);
    settings.setValue(QStringLiteral("AvatarPath"), m_avatarPath);
    settings.endGroup();
}
