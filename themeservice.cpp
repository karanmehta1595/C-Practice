// themeservice.cpp
#include "themeservice.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>

bool ThemeService::systemPrefersDark()
{
#ifdef Q_OS_WIN
    QSettings registry(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return registry.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
#endif
}

bool ThemeService::effectiveDark(AppTheme theme)
{
    if (theme == AppTheme::System)
        return systemPrefersDark();
    return theme == AppTheme::Dark;
}
