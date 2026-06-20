#include "themeservice.h"
#include <QSettings>

ThemeService* ThemeService::s_instance = nullptr;

ThemeService::ThemeService(QObject *parent) : QObject(parent) {
    s_instance = this;
    QSettings s("1OS", "Calculator");
    m_themeSetting = s.value("ThemeMode", 2).toInt();
}

ThemeService* ThemeService::instance() {
    if (!s_instance) s_instance = new ThemeService();
    return s_instance;
}

int ThemeService::getThemeSetting() const { return m_themeSetting; }

void ThemeService::setThemeSetting(int theme) {
    m_themeSetting = theme;
    QSettings("1OS", "Calculator").setValue("ThemeMode", theme);
    emit themeChanged();
}

QString ThemeService::getStyleSheet() const {
    int t = m_themeSetting;
    if (t == 2) {
        QSettings r("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
        t = (r.value("AppsUseLightTheme", 0).toInt() == 1) ? 0 : 1;
    }
    
    if (t == 1) {
        return "QWidget#centralWidget{background:rgba(22,22,24,92);border-radius:14px;border:1px solid rgba(255,255,255,0.15);}"
               "QWidget#topBarPanel{background:rgb(28,28,30);border-top-left-radius:13px;border-top-right-radius:13px;border-bottom:1px solid rgba(255,255,255,0.10);}"
               "QLabel#titleLabel{color:#F5F5F7;font-family:'Segoe UI';font-size:12px;font-weight:bold;}"
               "QWidget#displayCard{background:rgb(36,36,38);border:1px solid rgba(255,255,255,0.07);border-radius:14px;}"
               "QLabel#expressionLabel{color:#8E8E93;font-size:15px;background:transparent;}"
               "QLineEdit#displayBox{background:transparent;border:none;color:#FFFFFF;font-family:'Segoe UI';font-weight:300;padding:12px 0;}"
               "QLineEdit{background:#2C2C2E;color:white;border:1px solid rgba(255,255,255,0.1);border-radius:6px;padding-left:8px;}"
               "QListWidget#historyList{background:#0E0E10;border:1px solid rgba(255,255,255,0.07);border-radius:8px;color:#00FFCC;font-family:'Consolas';font-size:11px;}"
               "QPushButton{background:rgb(52,52,54);border:1px solid rgba(255,255,255,0.04);border-radius:8px;color:#E5E5EA;font-size:13px;font-weight:bold;min-height:36px;}"
               "QPushButton:hover{background:rgba(70,70,72,240);}"
               "QPushButton:pressed{background:rgba(90,90,92,240);}"
               "QPushButton#numButton{background:rgb(70,70,72);font-size:17px;font-weight:normal;}"
               "QPushButton#actionButton{background:rgb(242,115,12);color:white;border:none;font-size:15px;}"
               "QPushButton#actionButton:hover{background:rgb(255,135,40);}"
               "QPushButton#sysActionButton{background:rgb(68,32,32);color:#FF453A;border:1px solid rgb(108,48,48);}"
               "QPushButton#sciButton{background:rgb(44,44,46);color:#D1D1D6;font-size:12px;}"
               "QPushButton#clearHistoryBtn{background:rgba(180,40,40,180);color:white;border-radius:6px;border:none;font-size:11px;min-height:28px;}"
               "QPushButton#btnMin,QPushButton#btnMax,QPushButton#btnClose{border-radius:7px;min-width:14px;max-width:14px;min-height:14px;max-height:14px;padding:0px;margin:0px;color:transparent;}"
               "QPushButton#btnMin{background:#FEBC2E;}QPushButton#btnMax{background:#28C840;}QPushButton#btnClose{background:#FF5F57;}"
               "QPushButton#btnMin:hover{background:#FFD45A;}QPushButton#btnMax:hover{background:#44E55D;}QPushButton#btnClose:hover{background:#FF7C75;}"
               "#geoCard{background:rgba(44,44,46,200);border-radius:8px;}"
               "QScrollBar:vertical{width:5px;background:transparent;}QScrollBar::handle:vertical{background:rgba(255,255,255,0.2);border-radius:2px;}"
               "QComboBox{background:#3A3A3C;color:white;border-radius:4px;padding-left:4px;}"
               "QComboBox QAbstractItemView{background:#2C2C2E;color:white;selection-background-color:#FF9F0A;}"
               "QPushButton#toolbarTextBtn{background:transparent;border:none;color:#E5E5EA;font-size:12px;font-weight:500;padding:0 3px;border-radius:6px;}"
               "QPushButton#toolbarTextBtn:hover{background:rgba(255,255,255,0.16);}"
               "#toolbarFrame{background:rgb(60,60,66);border:1px solid rgba(255,255,255,0.25);border-radius:12px;padding:4px;}"
               "QLabel#toolbarSeparator{color:rgba(255,255,255,0.45);font-size:14px;font-weight:bold;padding:0 2px;}";
    } else {
        return "QWidget#centralWidget{background:rgba(248,248,250,88);border-radius:14px;border:1px solid rgba(0,0,0,0.10);}"
               "QWidget#topBarPanel{background:rgb(245,245,247);border-top-left-radius:13px;border-top-right-radius:13px;border-bottom:1px solid rgba(0,0,0,0.08);}"
               "QLabel#titleLabel{color:#1C1C1E;font-family:'Segoe UI';font-size:12px;font-weight:bold;}"
               "QWidget#displayCard{background:rgb(242,242,247);border:1px solid rgba(0,0,0,0.07);border-radius:14px;}"
               "QLabel#expressionLabel{color:#6D6D70;font-size:15px;background:transparent;}"
               "QLineEdit#displayBox{background:transparent;border:none;color:#1C1C1E;font-family:'Segoe UI';font-weight:300;padding:12px 0;}"
               "QLineEdit{background:#FFFFFF;color:#1C1C1E;border:1px solid rgba(0,0,0,0.1);border-radius:6px;padding-left:8px;}"
               "QListWidget#historyList{background:#FFFFFF;border:1px solid rgba(0,0,0,0.06);border-radius:8px;color:#007AFF;font-family:'Consolas';font-size:11px;}"
               "QPushButton{background:#FFFFFF;border:1px solid rgba(0,0,0,0.08);border-radius:8px;color:#1C1C1E;font-size:13px;font-weight:bold;min-height:36px;}"
               "QPushButton:hover{background:#EAEAED;}"
               "QPushButton:pressed{background:#D8D8DC;}"
               "QPushButton#numButton{background:#FFFFFF;font-size:17px;font-weight:normal;}"
               "QPushButton#actionButton{background:rgb(242,115,12);color:white;border:none;font-size:15px;}"
               "QPushButton#actionButton:hover{background:rgb(255,135,40);}"
               "QPushButton#sysActionButton{background:rgb(255,232,230);color:#FF3B30;border:1px solid rgb(255,185,180);}"
               "QPushButton#sciButton{background:rgb(228,228,232);color:#1C1C1E;font-size:12px;}"
               "QPushButton#clearHistoryBtn{background:rgba(255,59,48,200);color:white;border-radius:6px;border:none;font-size:11px;min-height:28px;}"
               "QPushButton#btnMin,QPushButton#btnMax,QPushButton#btnClose{border-radius:7px;min-width:14px;max-width:14px;min-height:14px;max-height:14px;padding:0px;margin:0px;color:transparent;}"
               "QPushButton#btnMin{background:#FEBC2E;}QPushButton#btnMax{background:#28C840;}QPushButton#btnClose{background:#FF5F57;}"
               "QPushButton#btnMin:hover{background:#FFD45A;}QPushButton#btnMax:hover{background:#44E55D;}QPushButton#btnClose:hover{background:#FF7C75;}"
               "#geoCard{background:rgba(230,230,234,200);border-radius:8px;}"
               "QScrollBar:vertical{width:5px;background:transparent;}QScrollBar::handle:vertical{background:rgba(0,0,0,0.2);border-radius:2px;}"
               "QComboBox{background:#E5E5EA;color:#1C1C1E;border-radius:4px;padding-left:4px;}"
               "QComboBox QAbstractItemView{background:#FFFFFF;color:#1C1C1E;selection-background-color:#FF9F0A;}"
               "QPushButton#toolbarTextBtn{background:transparent;border:none;color:#1C1C1E;font-size:12px;font-weight:500;padding:0 3px;border-radius:6px;}"
               "QPushButton#toolbarTextBtn:hover{background:rgba(0,0,0,0.08);}"
               "#toolbarFrame{background:rgba(245,245,247,230);border:1px solid rgba(0,0,0,0.08);border-radius:10px;}"
               "QLabel#toolbarSeparator{color:rgba(0,0,0,0.35);font-size:14px;font-weight:bold;padding:0 2px;}";
    }
}