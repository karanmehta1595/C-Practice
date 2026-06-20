// themeservice.h
#ifndef THEMESERVICE_H
#define THEMESERVICE_H

#include "settingsdialog.h"

class ThemeService
{
public:
    static bool systemPrefersDark();
    static bool effectiveDark(AppTheme theme);
};

#endif // THEMESERVICE_H
