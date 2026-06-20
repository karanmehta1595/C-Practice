// settingsdialog.h
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QSpinBox;

enum class AppTheme {
    Light,
    Dark,
    System
};

enum class StartupBehavior {
    ResumeLastNote,
    CreateBlankNote,
    ShowNotesList
};

struct AppSettings
{
    AppTheme theme = AppTheme::System;
    int editorFontSize = 12;
    int autoSaveIntervalSeconds = 2;
    StartupBehavior startupBehavior = StartupBehavior::ResumeLastNote;
    bool launchAtLogin = false;
    bool driveSyncEnabled = false;
    bool autoSortChecklistItems = false;

    static AppSettings load();
    void save() const;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    AppSettings settings() const;
    void setSettings(const AppSettings &settings);

signals:
    void settingsApplied(const AppSettings &settings);

private:
    void buildUI();
    void connectSignals();
    void applyAndAccept();
    void applyOnly();
    AppSettings collectSettings() const;
    void populateFromSettings(const AppSettings &settings);

    QComboBox *themeCombo = nullptr;
    QSpinBox *fontSizeSpin = nullptr;
    QSpinBox *autoSaveIntervalSpin = nullptr;
    QComboBox *startupBehaviorCombo = nullptr;
    QCheckBox *launchAtLoginCheck = nullptr;
    QCheckBox *autoSortChecklistCheck = nullptr;

    QLabel *syncStatusLabel = nullptr;
    QCheckBox *driveSyncCheck = nullptr;

    QDialogButtonBox *buttonBox = nullptr;
    AppSettings currentSettings;
};

#endif // SETTINGSDIALOG_H
