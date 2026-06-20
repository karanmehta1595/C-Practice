// main.cpp
#include "mainwindow.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setOrganizationName(QStringLiteral("1OS"));
    QApplication::setApplicationName(QStringLiteral("1OS Notes"));
    QApplication::setApplicationDisplayName(QStringLiteral("1OS Notes"));
    QApplication::setApplicationVersion(QStringLiteral("2.0.0"));

    QFont appFont(QStringLiteral("Segoe UI"), 10);
    QApplication::setFont(appFont);

    MainWindow window;
    window.show();

    return app.exec();
}
