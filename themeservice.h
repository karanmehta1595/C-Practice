#ifndef THEMESERVICE_H
#define THEMESERVICE_H

#include <QObject>
#include <QString>

class ThemeService : public QObject {
    Q_OBJECT
public:
    explicit ThemeService(QObject *parent = nullptr);
    static ThemeService* instance();

    int getThemeSetting() const;
    void setThemeSetting(int theme);
    QString getStyleSheet() const;

signals:
    void themeChanged();

private:
    static ThemeService* s_instance;
    int m_themeSetting = 2;
};

#endif // THEMESERVICE_H