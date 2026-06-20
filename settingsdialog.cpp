// settingsdialog.cpp
#include "settingsdialog.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

AppSettings AppSettings::load()
{
    QSettings settings;
    AppSettings result;

    settings.beginGroup(QStringLiteral("Appearance"));
    result.theme = static_cast<AppTheme>(
        settings.value(QStringLiteral("Theme"), static_cast<int>(AppTheme::System)).toInt());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Editor"));
    result.editorFontSize = settings.value(QStringLiteral("FontSize"), 12).toInt();
    result.autoSaveIntervalSeconds = settings.value(QStringLiteral("AutoSaveIntervalSeconds"), 2).toInt();
    result.autoSortChecklistItems = settings.value(QStringLiteral("AutoSortChecklistItems"), false).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Startup"));
    result.startupBehavior = static_cast<StartupBehavior>(
        settings.value(QStringLiteral("Behavior"),
                       static_cast<int>(StartupBehavior::ResumeLastNote)).toInt());
    result.launchAtLogin = settings.value(QStringLiteral("LaunchAtLogin"), false).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Sync"));
    result.driveSyncEnabled = settings.value(QStringLiteral("DriveSyncEnabled"), false).toBool();
    settings.endGroup();

    return result;
}

void AppSettings::save() const
{
    QSettings settings;

    settings.beginGroup(QStringLiteral("Appearance"));
    settings.setValue(QStringLiteral("Theme"), static_cast<int>(theme));
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Editor"));
    settings.setValue(QStringLiteral("FontSize"), editorFontSize);
    settings.setValue(QStringLiteral("AutoSaveIntervalSeconds"), autoSaveIntervalSeconds);
    settings.setValue(QStringLiteral("AutoSortChecklistItems"), autoSortChecklistItems);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Startup"));
    settings.setValue(QStringLiteral("Behavior"), static_cast<int>(startupBehavior));
    settings.setValue(QStringLiteral("LaunchAtLogin"), launchAtLogin);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Sync"));
    settings.setValue(QStringLiteral("DriveSyncEnabled"), driveSyncEnabled);
    settings.endGroup();
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    buildUI();
    connectSignals();
    setSettings(AppSettings::load());
}

AppSettings SettingsDialog::settings() const
{
    return currentSettings;
}

void SettingsDialog::setSettings(const AppSettings &settings)
{
    currentSettings = settings;
    populateFromSettings(settings);
}

void SettingsDialog::buildUI()
{
    setWindowTitle(QStringLiteral("Settings"));
    setModal(true);
    setMinimumWidth(460);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(16);

    auto *appearanceGroup = new QGroupBox(QStringLiteral("Appearance"), this);
    auto *appearanceForm = new QFormLayout(appearanceGroup);
    themeCombo = new QComboBox(appearanceGroup);
    themeCombo->addItem(QStringLiteral("Light"), static_cast<int>(AppTheme::Light));
    themeCombo->addItem(QStringLiteral("Dark"), static_cast<int>(AppTheme::Dark));
    themeCombo->addItem(QStringLiteral("System"), static_cast<int>(AppTheme::System));
    appearanceForm->addRow(QStringLiteral("Theme:"), themeCombo);

    auto *editorGroup = new QGroupBox(QStringLiteral("Editor"), this);
    auto *editorForm = new QFormLayout(editorGroup);
    fontSizeSpin = new QSpinBox(editorGroup);
    fontSizeSpin->setRange(9, 32);
    fontSizeSpin->setSuffix(QStringLiteral(" pt"));
    editorForm->addRow(QStringLiteral("Editor font size:"), fontSizeSpin);

    autoSaveIntervalSpin = new QSpinBox(editorGroup);
    autoSaveIntervalSpin->setRange(1, 120);
    autoSaveIntervalSpin->setSuffix(QStringLiteral(" sec"));
    editorForm->addRow(QStringLiteral("Auto save interval:"), autoSaveIntervalSpin);

    autoSortChecklistCheck = new QCheckBox(QStringLiteral("Automatically sort completed checklist items"), editorGroup);
    autoSortChecklistCheck->setToolTip(QStringLiteral("Stored as a preference for checklist workflows."));
    editorForm->addRow(QString(), autoSortChecklistCheck);

    auto *startupGroup = new QGroupBox(QStringLiteral("Startup"), this);
    auto *startupForm = new QFormLayout(startupGroup);
    startupBehaviorCombo = new QComboBox(startupGroup);
    startupBehaviorCombo->addItem(QStringLiteral("Resume last note"),
                                  static_cast<int>(StartupBehavior::ResumeLastNote));
    startupBehaviorCombo->addItem(QStringLiteral("Start with a blank note"),
                                  static_cast<int>(StartupBehavior::CreateBlankNote));
    startupBehaviorCombo->addItem(QStringLiteral("Show notes list"),
                                  static_cast<int>(StartupBehavior::ShowNotesList));
    startupForm->addRow(QStringLiteral("On startup:"), startupBehaviorCombo);

    launchAtLoginCheck = new QCheckBox(QStringLiteral("Launch 1OS Notes when I log in"), startupGroup);
    startupForm->addRow(QString(), launchAtLoginCheck);

    auto *syncGroup = new QGroupBox(QStringLiteral("Account && Sync"), this);
    auto *syncForm = new QFormLayout(syncGroup);
    syncStatusLabel = new QLabel(QStringLiteral("Not signed in"), syncGroup);
    syncForm->addRow(QStringLiteral("Status:"), syncStatusLabel);
    driveSyncCheck = new QCheckBox(QStringLiteral("Back up and sync notes with Google Drive"), syncGroup);
    driveSyncCheck->setEnabled(false);
    driveSyncCheck->setToolTip(QStringLiteral("Google sign-in is prepared in the code, but OAuth is not enabled in this build."));
    syncForm->addRow(QString(), driveSyncCheck);

    buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this);

    rootLayout->addWidget(appearanceGroup);
    rootLayout->addWidget(editorGroup);
    rootLayout->addWidget(startupGroup);
    rootLayout->addWidget(syncGroup);
    rootLayout->addWidget(buttonBox);
}

void SettingsDialog::connectSignals()
{
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::applyAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if (buttonBox->buttonRole(button) == QDialogButtonBox::ApplyRole)
            applyOnly();
    });
}

void SettingsDialog::applyAndAccept()
{
    applyOnly();
    accept();
}

void SettingsDialog::applyOnly()
{
    currentSettings = collectSettings();
    currentSettings.save();
    emit settingsApplied(currentSettings);
}

AppSettings SettingsDialog::collectSettings() const
{
    AppSettings settings;
    settings.theme = static_cast<AppTheme>(themeCombo->currentData().toInt());
    settings.editorFontSize = fontSizeSpin->value();
    settings.autoSaveIntervalSeconds = autoSaveIntervalSpin->value();
    settings.startupBehavior =
        static_cast<StartupBehavior>(startupBehaviorCombo->currentData().toInt());
    settings.launchAtLogin = launchAtLoginCheck->isChecked();
    settings.driveSyncEnabled = driveSyncCheck->isChecked();
    settings.autoSortChecklistItems = autoSortChecklistCheck->isChecked();
    return settings;
}

void SettingsDialog::populateFromSettings(const AppSettings &settings)
{
    themeCombo->setCurrentIndex(qMax(0, themeCombo->findData(static_cast<int>(settings.theme))));
    fontSizeSpin->setValue(settings.editorFontSize);
    autoSaveIntervalSpin->setValue(settings.autoSaveIntervalSeconds);
    startupBehaviorCombo->setCurrentIndex(
        qMax(0, startupBehaviorCombo->findData(static_cast<int>(settings.startupBehavior))));
    launchAtLoginCheck->setChecked(settings.launchAtLogin);
    driveSyncCheck->setChecked(settings.driveSyncEnabled);
    autoSortChecklistCheck->setChecked(settings.autoSortChecklistItems);
}
