// mainwindow.cpp
#include "mainwindow.h"

#include "attachmentmanager.h"
#include "foldermanager.h"
#include "loginmanager.h"
#include "noteseditorwidget.h"
#include "noteslistwidget.h"
#include "searchengine.h"
#include "smartfoldermanager.h"
#include "sidebarwidget.h"
#include "tagmanager.h"
#include "templatemanager.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPageSize>
#include <QPalette>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSplitter>
#include <QTextDocument>
#include <QTextStream>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace {
QPushButton *makeButton(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

bool systemPrefersDark()
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

bool isTitleBarDragArea(MainWindow *window, QWidget *titleBar, const QPoint &windowPos)
{
    if (!window || !titleBar)
        return false;

    const QPoint titleBarPos = titleBar->mapFrom(window, windowPos);
    if (!titleBar->rect().contains(titleBarPos))
        return false;

    QWidget *child = window->childAt(windowPos);
    while (child && child != titleBar) {
        if (qobject_cast<QAbstractButton *>(child) || qobject_cast<QLineEdit *>(child))
            return false;
        child = child->parentWidget();
    }
    return true;
}

bool noteContains(const Note &note, const QString &query, bool showLockedContent)
{
    const QString needle = query.trimmed();
    if (needle.isEmpty())
        return true;

    if (note.title.contains(needle, Qt::CaseInsensitive)
        || note.tags.join(QLatin1Char(' ')).contains(needle, Qt::CaseInsensitive)) {
        return true;
    }

    for (const NoteAttachment &attachment : note.attachments) {
        if (attachment.displayName.contains(needle, Qt::CaseInsensitive)
            || attachment.filePath.contains(needle, Qt::CaseInsensitive)) {
            return true;
        }
    }

    if (note.locked && !showLockedContent)
        return false;

    return note.body.contains(needle, Qt::CaseInsensitive)
           || note.html.contains(needle, Qt::CaseInsensitive);
}

QString cleanedTagsText(const QString &text)
{
    QStringList tags;
    for (const QString &raw : text.split(QRegularExpression(QStringLiteral("[,\\s]+")),
                                        Qt::SkipEmptyParts)) {
        QString tag = raw.trimmed();
        if (tag.startsWith(QLatin1Char('#')))
            tag.remove(0, 1);
        if (!tag.isEmpty() && !tags.contains(tag, Qt::CaseInsensitive))
            tags.append(tag);
    }
    tags.sort(Qt::CaseInsensitive);
    return tags.join(QStringLiteral(", "));
}

QString notesStyleSheet(bool dark)
{
    if (dark) {
        return QString::fromLatin1(R"(
QWidget#centralWidget {
    background: #1B1B1D;
    border: 1px solid #343438;
    border-radius: 14px;
}
QWidget#titleBar {
    background: #2A2A2D;
    border-top-left-radius: 13px;
    border-top-right-radius: 13px;
    border-bottom: 1px solid #38383D;
}
QLabel#titleLabel {
    color: #F5F5F7;
    font-size: 13px;
    font-weight: 700;
}
QWidget#contentSurface {
    background: #1B1B1D;
    border-bottom-left-radius: 13px;
    border-bottom-right-radius: 13px;
}
#sidebarPanel {
    background: #242427;
    border-bottom-left-radius: 13px;
}
#notesPanel {
    background: #202023;
    border-left: 1px solid #343438;
    border-right: 1px solid #343438;
}
#editorPanel {
    background: #18181A;
    border-bottom-right-radius: 13px;
}
QLabel#sidebarTitleLabel,
QLabel#notesHeaderTitle {
    color: #F5F5F7;
    font-size: 18px;
    font-weight: 800;
}
QLabel#notesMetaLabel,
QLabel#statusLabel,
QLabel#attachmentsLabel {
    color: #A6A6AD;
    font-size: 12px;
}
QListWidget {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget#sidebarList::item {
    color: #D7D7DC;
    min-height: 32px;
    padding: 3px 10px;
    border-radius: 8px;
}
QListWidget#sidebarList::item:hover {
    background: #303035;
}
QListWidget#sidebarList::item:selected {
    background: #423819;
    color: #FFD763;
}
QListWidget#notesList::item {
    background: transparent;
    border: none;
}
QWidget#noteCard {
    background: #29292D;
    border: 1px solid #37373D;
    border-radius: 8px;
}
QWidget#noteCard:hover {
    background: #303036;
    border-color: #505058;
}
QWidget#noteCard[selected="true"] {
    background: #413719;
    border: 1px solid #D4A72C;
}
QLabel#noteTitleLabel {
    color: #F5F5F7;
    font-size: 14px;
    font-weight: 800;
}
QLabel#notePreviewLabel {
    color: #C9C9CF;
    font-size: 12px;
}
QLabel#noteDateLabel,
QLabel#noteTagsLabel,
QLabel#noteStateLabel {
    color: #9A9AA2;
    font-size: 11px;
}
QLineEdit, QComboBox {
    background: #2A2A2D;
    color: #F5F5F7;
    border: 1px solid #3A3A40;
    border-radius: 8px;
    padding: 6px 9px;
    selection-background-color: #D4A72C;
}
QLineEdit:focus, QComboBox:focus {
    border-color: #D4A72C;
}
QLineEdit#titleSearchBox {
    border-radius: 12px;
    padding: 3px 10px;
}
QLineEdit#noteTitleField {
    background: transparent;
    border: none;
    color: #FFFFFF;
    font-size: 28px;
    font-weight: 800;
    padding: 8px 2px 4px 2px;
}
QTextEdit#editor {
    background: #202023;
    color: #F5F5F7;
    border: 1px solid #343438;
    border-radius: 8px;
    padding: 20px;
    font-size: 15px;
    selection-background-color: #8F6D10;
}
QWidget#editorToolbar {
    background: transparent;
    border-bottom: 1px solid #343438;
}
QWidget#statusBarPanel {
    background: #202023;
    border: 1px solid #303035;
    border-radius: 8px;
}
QPushButton {
    background: #303035;
    color: #F2F2F7;
    border: 1px solid #3E3E44;
    border-radius: 8px;
    padding: 6px 10px;
    font-weight: 700;
}
QPushButton:hover {
    background: #3A3A40;
}
QPushButton#primaryActionButton {
    background: #D4A72C;
    color: #171717;
    border: none;
}
QPushButton#secondaryActionButton,
QPushButton#sidebarActionButton,
QPushButton#titleActionButton,
QPushButton#formatButton,
QPushButton#dangerButton {
    background: transparent;
    border: none;
    color: #A8A8B0;
    padding: 4px 6px;
}
QPushButton#formatButton:hover,
QPushButton#titleActionButton:hover,
QPushButton#sidebarActionButton:hover {
    color: #FFFFFF;
    background: #303035;
}
QPushButton#dangerButton:hover {
    color: #FF8A85;
    background: #3A2525;
}
QPushButton#btnClose,
QPushButton#btnMin,
QPushButton#btnMax {
    min-width: 14px;
    max-width: 14px;
    min-height: 14px;
    max-height: 14px;
    border-radius: 7px;
    border: none;
    padding: 0;
    margin: 0;
    color: transparent;
}
QPushButton#btnClose { background: #FF5F57; }
QPushButton#btnMin { background: #FEBC2E; }
QPushButton#btnMax { background: #28C840; }
QPushButton#btnClose:hover { background: #FF7C75; color: #5C0000; }
QPushButton#btnMin:hover { background: #FFD45A; color: #684300; }
QPushButton#btnMax:hover { background: #44E55D; color: #005C17; }
QSplitter::handle { background: #343438; }
QScrollBar:vertical { width: 10px; background: transparent; }
QScrollBar::handle:vertical { background: #4B4B52; border-radius: 5px; min-height: 34px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)");
    }

    return QString::fromLatin1(R"(
QWidget#centralWidget {
    background: #F6F6F8;
    border: 1px solid #D8D8DE;
    border-radius: 14px;
}
QWidget#titleBar {
    background: #F2F2F5;
    border-top-left-radius: 13px;
    border-top-right-radius: 13px;
    border-bottom: 1px solid #DCDCE2;
}
QLabel#titleLabel {
    color: #1C1C1E;
    font-size: 13px;
    font-weight: 700;
}
QWidget#contentSurface {
    background: #F6F6F8;
    border-bottom-left-radius: 13px;
    border-bottom-right-radius: 13px;
}
#sidebarPanel {
    background: #ECECF1;
    border-bottom-left-radius: 13px;
}
#notesPanel {
    background: #F7F7FA;
    border-left: 1px solid #DEDEE4;
    border-right: 1px solid #DEDEE4;
}
#editorPanel {
    background: #FFFDF7;
    border-bottom-right-radius: 13px;
}
QLabel#sidebarTitleLabel,
QLabel#notesHeaderTitle {
    color: #1C1C1E;
    font-size: 18px;
    font-weight: 800;
}
QLabel#notesMetaLabel,
QLabel#statusLabel,
QLabel#attachmentsLabel {
    color: #6E6E76;
    font-size: 12px;
}
QListWidget {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget#sidebarList::item {
    color: #303036;
    min-height: 32px;
    padding: 3px 10px;
    border-radius: 8px;
}
QListWidget#sidebarList::item:hover {
    background: #E1E1E7;
}
QListWidget#sidebarList::item:selected {
    background: #FFE8A3;
    color: #1C1C1E;
}
QListWidget#notesList::item {
    background: transparent;
    border: none;
}
QWidget#noteCard {
    background: #FFFFFF;
    border: 1px solid #E1E1E7;
    border-radius: 8px;
}
QWidget#noteCard:hover {
    background: #FFF8E6;
    border-color: #E7D08A;
}
QWidget#noteCard[selected="true"] {
    background: #FFEFC2;
    border: 1px solid #D8A800;
}
QLabel#noteTitleLabel {
    color: #1C1C1E;
    font-size: 14px;
    font-weight: 800;
}
QLabel#notePreviewLabel {
    color: #4E4E56;
    font-size: 12px;
}
QLabel#noteDateLabel,
QLabel#noteTagsLabel,
QLabel#noteStateLabel {
    color: #777780;
    font-size: 11px;
}
QLineEdit, QComboBox {
    background: #FFFFFF;
    color: #1C1C1E;
    border: 1px solid #DADAE0;
    border-radius: 8px;
    padding: 6px 9px;
    selection-background-color: #FFD763;
}
QLineEdit:focus, QComboBox:focus {
    border-color: #D8A800;
}
QLineEdit#titleSearchBox {
    border-radius: 12px;
    padding: 3px 10px;
}
QLineEdit#noteTitleField {
    background: transparent;
    border: none;
    color: #111113;
    font-size: 28px;
    font-weight: 800;
    padding: 8px 2px 4px 2px;
}
QTextEdit#editor {
    background: #FFFFFF;
    color: #1C1C1E;
    border: 1px solid #E0E0E6;
    border-radius: 8px;
    padding: 20px;
    font-size: 15px;
    selection-background-color: #FFE08A;
}
QWidget#editorToolbar {
    background: transparent;
    border-bottom: 1px solid #DEDEE4;
}
QWidget#statusBarPanel {
    background: #FFFDF7;
    border: 1px solid #E6E1D3;
    border-radius: 8px;
}
QPushButton {
    background: #FFFFFF;
    color: #1C1C1E;
    border: 1px solid #DADAE0;
    border-radius: 8px;
    padding: 6px 10px;
    font-weight: 700;
}
QPushButton:hover {
    background: #F1F1F4;
}
QPushButton#primaryActionButton {
    background: #FFD34E;
    color: #1C1C1E;
    border: none;
}
QPushButton#secondaryActionButton,
QPushButton#sidebarActionButton,
QPushButton#titleActionButton,
QPushButton#formatButton,
QPushButton#dangerButton {
    background: transparent;
    border: none;
    color: #6E6E73;
    padding: 4px 6px;
}
QPushButton#formatButton:hover,
QPushButton#titleActionButton:hover,
QPushButton#sidebarActionButton:hover {
    color: #1C1C1E;
    background: #E7E7EC;
}
QPushButton#dangerButton:hover {
    color: #C7352E;
    background: #FFE9E8;
}
QPushButton#btnClose,
QPushButton#btnMin,
QPushButton#btnMax {
    min-width: 14px;
    max-width: 14px;
    min-height: 14px;
    max-height: 14px;
    border-radius: 7px;
    border: none;
    padding: 0;
    margin: 0;
    color: transparent;
}
QPushButton#btnClose { background: #FF5F57; }
QPushButton#btnMin { background: #FEBC2E; }
QPushButton#btnMax { background: #28C840; }
QPushButton#btnClose:hover { background: #FF7C75; color: #5C0000; }
QPushButton#btnMin:hover { background: #FFD45A; color: #684300; }
QPushButton#btnMax:hover { background: #44E55D; color: #005C17; }
QSplitter::handle { background: #DEDEE4; }
QScrollBar:vertical { width: 10px; background: transparent; }
QScrollBar::handle:vertical { background: #C4C4CC; border-radius: 5px; min-height: 34px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)");
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , notesManager(new NotesManager(this))
    , loginManager(new LoginManager(this))
{
    setWindowTitle(QStringLiteral("1OS Notes"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowMinMaxButtonsHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMinimumSize(1080, 680);
    resize(1280, 780);

    appSettings = AppSettings::load();
    notesManager->setAutoSaveIntervalMs(appSettings.autoSaveIntervalSeconds * 1000);
    notesEditorWidget = nullptr;

    notesManager->load();
    loginManager->restoreSession();

    buildUI();
    connectSignals();
    applySettings(appSettings);
    ensureStarterContent();
    refreshSidebar();

    if (appSettings.startupBehavior == StartupBehavior::CreateBlankNote)
        createNewNote();
    else
        refreshNotesList();
}

MainWindow::~MainWindow()
{
    notesManager->flushPendingChanges();
}

void MainWindow::applyCurrentTheme()
{
    const bool dark = currentTheme == SystemTheme ? systemPrefersDark() : currentTheme == DarkTheme;
    setStyleSheet(notesStyleSheet(dark));
}

void MainWindow::setTheme(ThemeMode theme)
{
    currentTheme = theme;
    applyCurrentTheme();
}

void MainWindow::applySettings(const AppSettings &settings)
{
    appSettings = settings;
    notesManager->setAutoSaveIntervalMs(settings.autoSaveIntervalSeconds * 1000);
    notesEditorWidget->setEditorFontSize(settings.editorFontSize);

    switch (settings.theme) {
    case AppTheme::Light:
        setTheme(LightTheme);
        break;
    case AppTheme::Dark:
        setTheme(DarkTheme);
        break;
    case AppTheme::System:
        setTheme(SystemTheme);
        break;
    }
}

void MainWindow::buildUI()
{
    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("centralWidget"));

    auto *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    buildTitleBar();
    rootLayout->addWidget(titleBar);

    auto *contentSurface = new QWidget(centralWidget);
    contentSurface->setObjectName(QStringLiteral("contentSurface"));

    auto *contentLayout = new QHBoxLayout(contentSurface);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, contentSurface);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);

    sidebarWidget = new SidebarWidget(splitter);
    notesListWidget = new NotesListWidget(splitter);
    notesEditorWidget = new NotesEditorWidget(splitter);

    splitter->addWidget(sidebarWidget);
    splitter->addWidget(notesListWidget);
    splitter->addWidget(notesEditorWidget);
    splitter->setSizes({240, 330, 710});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 0);
    splitter->setStretchFactor(2, 1);

    contentLayout->addWidget(splitter);
    rootLayout->addWidget(contentSurface, 1);
    setCentralWidget(centralWidget);
}

void MainWindow::buildTitleBar()
{
    titleBar = new QWidget(centralWidget);
    titleBar->setObjectName(QStringLiteral("titleBar"));
    titleBar->setFixedHeight(30);

    auto *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(14, 0, 14, 0);
    titleLayout->setSpacing(0);

    auto *leftCluster = new QWidget(titleBar);
    leftCluster->setFixedWidth(176);
    auto *leftLayout = new QHBoxLayout(leftCluster);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    btnClose = makeButton(QStringLiteral("x"), QStringLiteral("btnClose"), leftCluster);
    btnClose->setToolTip(QStringLiteral("Quit"));
    btnMin = makeButton(QStringLiteral("-"), QStringLiteral("btnMin"), leftCluster);
    btnMin->setToolTip(QStringLiteral("Minimise"));
    btnMax = makeButton(QStringLiteral("+"), QStringLiteral("btnMax"), leftCluster);
    btnMax->setToolTip(QStringLiteral("Maximise"));

    leftLayout->addWidget(btnClose);
    leftLayout->addWidget(btnMin);
    leftLayout->addWidget(btnMax);
    leftLayout->addStretch();

    titleLabel = new QLabel(QStringLiteral("Notes"), titleBar);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *rightCluster = new QWidget(titleBar);
    rightCluster->setFixedWidth(360);
    auto *rightLayout = new QHBoxLayout(rightCluster);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    btnSearch = makeButton(QStringLiteral("Search"), QStringLiteral("titleActionButton"), rightCluster);
    btnSettings = makeButton(QStringLiteral("Settings"), QStringLiteral("titleActionButton"), rightCluster);
    btnAccount = makeButton(QStringLiteral("Account"), QStringLiteral("titleActionButton"), rightCluster);

    titleSearchBox = new QLineEdit(rightCluster);
    titleSearchBox->setObjectName(QStringLiteral("titleSearchBox"));
    titleSearchBox->setPlaceholderText(QStringLiteral("Search notes"));
    titleSearchBox->setFixedSize(140, 22);
    titleSearchBox->hide();
    titleSearchBox->installEventFilter(this);

    rightLayout->addStretch();
    rightLayout->addWidget(titleSearchBox);
    rightLayout->addWidget(btnSearch);
    rightLayout->addWidget(btnSettings);
    rightLayout->addWidget(btnAccount);

    titleLayout->addWidget(leftCluster);
    titleLayout->addWidget(titleLabel, 1);
    titleLayout->addWidget(rightCluster);
}

void MainWindow::connectSignals()
{
    connect(btnClose, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(btnMin, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(btnMax, &QPushButton::clicked, this, [this]() {
        if (isMaximized()) {
            showNormal();
            btnMax->setToolTip(QStringLiteral("Maximise"));
        } else {
            showMaximized();
            btnMax->setToolTip(QStringLiteral("Restore"));
        }
    });

    connect(btnSearch, &QPushButton::clicked, this, [this]() {
        btnSearch->hide();
        titleSearchBox->show();
        titleSearchBox->setFocus();
        titleSearchBox->selectAll();
    });
    connect(btnSettings, &QPushButton::clicked, this, &MainWindow::showSettingsDialog);
    connect(btnAccount, &QPushButton::clicked, this, &MainWindow::showAccountMenu);
    connect(titleSearchBox, &QLineEdit::textChanged, this, [this]() {
        refreshNotesList(currentNoteId);
    });

    connect(sidebarWidget, &SidebarWidget::filterChanged, this,
            [this](const QString &kind, const QString &value) {
                currentFilterKind = kind;
                currentFilterValue = value;
                refreshNotesList();
            });
    connect(sidebarWidget, &SidebarWidget::newFolderRequested, this, &MainWindow::createFolder);
    connect(sidebarWidget, &SidebarWidget::newSmartFolderRequested, this, &MainWindow::createSmartFolder);

    connect(notesListWidget, &NotesListWidget::newNoteRequested, this, &MainWindow::createNewNote);
    connect(notesListWidget, &NotesListWidget::searchChanged, this, [this]() {
        refreshNotesList(currentNoteId);
    });
    connect(notesListWidget, &NotesListWidget::noteSelected, this, &MainWindow::loadCurrentNote);
    connect(notesListWidget, &NotesListWidget::pinRequested, this,
            [this](const QString &noteId, bool pinned) {
                notesManager->setPinned(noteId, pinned);
                refreshSidebar();
                refreshNotesList(noteId);
            });
    connect(notesListWidget, &NotesListWidget::lockRequested, this, &MainWindow::handleLockAction);
    connect(notesListWidget, &NotesListWidget::moveToTrashRequested, this,
            [this](const QString &noteId) {
                currentNoteId = noteId;
                moveCurrentNoteToTrash();
            });
    connect(notesListWidget, &NotesListWidget::restoreRequested, this, &MainWindow::restoreNote);
    connect(notesListWidget, &NotesListWidget::purgeRequested, this, &MainWindow::purgeNote);
    connect(notesListWidget, &NotesListWidget::exportRequested, this, &MainWindow::exportNote);

    connect(notesEditorWidget, &NotesEditorWidget::contentChanged, this,
            [this](const QString &title, const QString &body, const QString &html, const QStringList &tags) {
                if (currentNoteId.isEmpty() || currentNoteIsLockedAndHidden())
                    return;
                notesManager->updateNoteContent(currentNoteId, title, body, html, tags);
            });
    connect(notesEditorWidget, &NotesEditorWidget::attachRequested,
            this, &MainWindow::attachFileToCurrentNote);
    connect(notesEditorWidget, &NotesEditorWidget::lockRequested,
            this, [this]() { handleLockAction(currentNoteId); });
    connect(notesEditorWidget, &NotesEditorWidget::deleteRequested,
            this, &MainWindow::moveCurrentNoteToTrash);
    connect(notesEditorWidget, &NotesEditorWidget::exportRequested,
            this, [this]() { exportNote(currentNoteId); });
    connect(notesEditorWidget, &NotesEditorWidget::printRequested,
            this, [this]() { printNote(currentNoteId); });

    connect(notesManager, &NotesManager::folderCreated, this, [this]() {
        refreshSidebar();
        refreshNotesList(currentNoteId);
    });
    connect(notesManager, &NotesManager::folderDeleted, this, [this]() {
        refreshSidebar();
        refreshNotesList(currentNoteId);
    });
    connect(notesManager, &NotesManager::smartFolderCreated, this, [this]() {
        refreshSidebar();
        refreshNotesList(currentNoteId);
    });
    connect(notesManager, &NotesManager::autoSaveStarted, this, [this]() {
        notesEditorWidget->setAutosaveText(QStringLiteral("Autosaving..."));
    });
    connect(notesManager, &NotesManager::autoSaveFinished, this, [this](bool success) {
        notesEditorWidget->setAutosaveText(success ? QStringLiteral("Autosaved")
                                                   : QStringLiteral("Save failed"));
    });
}

void MainWindow::ensureStarterContent()
{
    if (!notesManager->allNotes().isEmpty() || !notesManager->trashedNotes().isEmpty())
        return;

    const QString ideasFolder = notesManager->createFolder(QStringLiteral("Ideas"));
    const QString workFolder = notesManager->createFolder(QStringLiteral("Work"));

    const QString welcome = notesManager->createNote(QStringLiteral("Welcome to 1OS Notes"),
                                                     QStringLiteral("A polished local notes space with rich text, folders, tags, smart folders, attachments, locks, import, export and print.\n\n#home #welcome"),
                                                     ideasFolder);
    notesManager->updateNoteContent(
        welcome,
        QStringLiteral("Welcome to 1OS Notes"),
        QStringLiteral("A polished local notes space with rich text, folders, tags, smart folders, attachments, locks, import, export and print.\n\n#home #welcome"),
        QStringLiteral("<h1>Welcome to 1OS Notes</h1><p>A polished local notes space with rich text, folders, tags, smart folders, attachments, locks, import, export and print.</p><ul><li>Use tags like <b>#home</b> and <b>#welcome</b>.</li><li>Create Smart Folders from the sidebar.</li><li>Attach files and export notes from the editor toolbar.</li></ul><p>#home #welcome</p>"),
        {QStringLiteral("home"), QStringLiteral("welcome")});
    notesManager->setPinned(welcome, true);

    const QString project = notesManager->createNote(QStringLiteral("Project Plan"),
                                                     QStringLiteral("Goals\n- Define scope\n- Prepare launch checklist\n- Export notes as PDF when needed\n\n#work #important"),
                                                     workFolder);
    notesManager->updateNoteContent(project,
                                    QStringLiteral("Project Plan"),
                                    QStringLiteral("Goals\n- Define scope\n- Prepare launch checklist\n- Export notes as PDF when needed\n\n#work #important"),
                                    QStringLiteral("<h2>Goals</h2><ul><li>Define scope</li><li>Prepare launch checklist</li><li>Export notes as PDF when needed</li></ul><p>#work #important</p>"),
                                    {QStringLiteral("work"), QStringLiteral("important")});

    notesManager->createSmartFolder(QStringLiteral("Important"),
                                    {QStringLiteral("important")},
                                    false);
    notesManager->flushPendingChanges();
}

void MainWindow::refreshSidebar()
{
    sidebarWidget->setLibrary(notesManager->allFolders(),
                              notesManager->allSmartFolders(),
                              notesManager->allTags(),
                              notesManager->allNotes().size(),
                              notesManager->pinnedNotes().size(),
                              notesManager->attachmentNotes().size(),
                              notesManager->lockedNotes().size(),
                              notesManager->trashedNotes().size());
    sidebarWidget->selectFilter(currentFilterKind, currentFilterValue);
}

void MainWindow::refreshNotesList(const QString &preferredNoteId)
{
    QList<Note> notes = notesForCurrentFilter();
    const QString globalQuery = titleSearchBox->isVisible() ? titleSearchBox->text().trimmed() : QString();
    const QString listQuery = notesListWidget->searchText();

    QList<Note> filtered;
    for (const Note &note : notes) {
        const bool showLocked = unlockedNoteIds.contains(note.id);
        if (noteContains(note, globalQuery, showLocked) && noteContains(note, listQuery, showLocked))
            filtered.append(note);
    }

    notesListWidget->setHeader(currentFilterTitle(), filtered.size());
    notesListWidget->setNotes(filtered, preferredNoteId.isEmpty() ? currentNoteId : preferredNoteId);

    if (filtered.isEmpty()) {
        currentNoteId.clear();
        notesEditorWidget->clearEditor();
    }
}

void MainWindow::loadCurrentNote(const QString &noteId)
{
    if (noteId.isEmpty() || !notesManager->hasNote(noteId))
        return;

    currentNoteId = noteId;
    notesManager->touchRecent(noteId);
    const Note note = notesManager->note(noteId);
    const bool locked = note.locked && !unlockedNoteIds.contains(noteId);
    notesEditorWidget->loadNote(note, locked);
}

QList<Note> MainWindow::notesForCurrentFilter() const
{
    if (currentFilterKind == QStringLiteral("pinned"))
        return notesManager->pinnedNotes();
    if (currentFilterKind == QStringLiteral("recent"))
        return notesManager->recentNotes(50);
    if (currentFilterKind == QStringLiteral("attachments"))
        return notesManager->attachmentNotes();
    if (currentFilterKind == QStringLiteral("locked"))
        return notesManager->lockedNotes();
    if (currentFilterKind == QStringLiteral("trash"))
        return notesManager->trashedNotes();
    if (currentFilterKind == QStringLiteral("folder"))
        return notesManager->notesInFolder(currentFilterValue);
    if (currentFilterKind == QStringLiteral("smart"))
        return notesManager->notesInSmartFolder(currentFilterValue);
    if (currentFilterKind == QStringLiteral("tag"))
        return notesManager->notesWithTag(currentFilterValue);
    return notesManager->allNotes();
}

QString MainWindow::currentFilterTitle() const
{
    if (currentFilterKind == QStringLiteral("pinned"))
        return QStringLiteral("Pinned");
    if (currentFilterKind == QStringLiteral("recent"))
        return QStringLiteral("Recent");
    if (currentFilterKind == QStringLiteral("attachments"))
        return QStringLiteral("Attachments");
    if (currentFilterKind == QStringLiteral("locked"))
        return QStringLiteral("Locked Notes");
    if (currentFilterKind == QStringLiteral("trash"))
        return QStringLiteral("Recently Deleted");
    if (currentFilterKind == QStringLiteral("tag"))
        return QStringLiteral("#%1").arg(currentFilterValue);
    if (currentFilterKind == QStringLiteral("folder")) {
        for (const NoteFolder &folder : notesManager->allFolders()) {
            if (folder.id == currentFilterValue)
                return folder.name;
        }
    }
    if (currentFilterKind == QStringLiteral("smart")) {
        const SmartFolder folder = notesManager->smartFolder(currentFilterValue);
        if (!folder.id.isEmpty())
            return folder.name;
    }
    return QStringLiteral("All Notes");
}

bool MainWindow::currentNoteIsLockedAndHidden() const
{
    if (currentNoteId.isEmpty())
        return false;
    const Note note = notesManager->note(currentNoteId);
    return note.locked && !unlockedNoteIds.contains(currentNoteId);
}

void MainWindow::createNewNote()
{
    QString folderId;
    if (currentFilterKind == QStringLiteral("folder"))
        folderId = currentFilterValue;

    const QString noteId = notesManager->createNote(QStringLiteral("Untitled Note"),
                                                    QString(),
                                                    folderId);
    currentFilterKind = QStringLiteral("all");
    currentFilterValue.clear();
    refreshSidebar();
    refreshNotesList(noteId);
    notesListWidget->selectNote(noteId);
    notesEditorWidget->focusTitleAndSelectAll();
}

void MainWindow::createFolder()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("New Folder"),
                                               QStringLiteral("Folder name:"),
                                               QLineEdit::Normal,
                                               QStringLiteral("New Folder"), &ok);
    if (!ok)
        return;

    const QString folderId = notesManager->createFolder(name);
    currentFilterKind = QStringLiteral("folder");
    currentFilterValue = folderId;
    refreshSidebar();
    refreshNotesList();
}

void MainWindow::createSmartFolder()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("New Smart Folder"),
                                               QStringLiteral("Smart folder name:"),
                                               QLineEdit::Normal,
                                               QStringLiteral("Smart Folder"), &ok);
    if (!ok)
        return;

    const QString tagText = QInputDialog::getText(this, QStringLiteral("Smart Folder Tags"),
                                                  QStringLiteral("Tags, separated by comma or space:"),
                                                  QLineEdit::Normal,
                                                  QString(), &ok);
    if (!ok)
        return;

    const QString cleaned = cleanedTagsText(tagText);
    const QStringList tags = cleaned.split(QStringLiteral(", "), Qt::SkipEmptyParts);
    const QString id = notesManager->createSmartFolder(name, tags, false);
    currentFilterKind = QStringLiteral("smart");
    currentFilterValue = id;
    refreshSidebar();
    refreshNotesList();
}

void MainWindow::attachFileToCurrentNote()
{
    if (currentNoteId.isEmpty() || currentNoteIsLockedAndHidden())
        return;

    const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("Attach File"));
    if (filePath.isEmpty())
        return;

    if (!notesManager->addAttachment(currentNoteId, filePath))
        return;

    const Note note = notesManager->note(currentNoteId);
    if (!note.attachments.isEmpty())
        notesEditorWidget->insertAttachmentReference(note.attachments.last());
    refreshSidebar();
    refreshNotesList(currentNoteId);
    loadCurrentNote(currentNoteId);
}

void MainWindow::handleLockAction(const QString &noteId)
{
    const QString id = noteId.isEmpty() ? currentNoteId : noteId;
    if (id.isEmpty() || !notesManager->hasNote(id))
        return;

    const Note note = notesManager->note(id);
    if (!note.locked) {
        bool ok = false;
        const QString password = QInputDialog::getText(this, QStringLiteral("Lock Note"),
                                                       QStringLiteral("Password:"),
                                                       QLineEdit::Password,
                                                       QString(), &ok);
        if (!ok || password.isEmpty())
            return;

        const QString hint = QInputDialog::getText(this, QStringLiteral("Password Hint"),
                                                   QStringLiteral("Hint (optional):"),
                                                   QLineEdit::Normal,
                                                   QString(), &ok);
        if (!ok)
            return;

        notesManager->lockNote(id, password, hint);
        unlockedNoteIds.remove(id);
        refreshSidebar();
        refreshNotesList(id);
        loadCurrentNote(id);
        return;
    }

    if (!unlockedNoteIds.contains(id)) {
        bool ok = false;
        QString prompt = QStringLiteral("Password:");
        if (!note.passwordHint.isEmpty())
            prompt += QStringLiteral("\nHint: %1").arg(note.passwordHint);
        const QString password = QInputDialog::getText(this, QStringLiteral("Unlock Note"),
                                                       prompt,
                                                       QLineEdit::Password,
                                                       QString(), &ok);
        if (!ok)
            return;
        if (!notesManager->unlockNote(id, password)) {
            QMessageBox::warning(this, QStringLiteral("Unlock Failed"),
                                 QStringLiteral("That password did not unlock the note."));
            return;
        }
        unlockedNoteIds.insert(id);
        loadCurrentNote(id);
        refreshNotesList(id);
        return;
    }

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Locked Note"));
    box.setText(QStringLiteral("What would you like to do with this locked note?"));
    QPushButton *closeButton = box.addButton(QStringLiteral("Close Lock"), QMessageBox::AcceptRole);
    QPushButton *removeButton = box.addButton(QStringLiteral("Remove Lock"), QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();

    if (box.clickedButton() == closeButton) {
        unlockedNoteIds.remove(id);
        loadCurrentNote(id);
        refreshNotesList(id);
    } else if (box.clickedButton() == removeButton) {
        bool ok = false;
        const QString password = QInputDialog::getText(this, QStringLiteral("Remove Lock"),
                                                       QStringLiteral("Password:"),
                                                       QLineEdit::Password,
                                                       QString(), &ok);
        if (!ok)
            return;
        if (!notesManager->removeLock(id, password)) {
            QMessageBox::warning(this, QStringLiteral("Remove Lock Failed"),
                                 QStringLiteral("That password did not match."));
            return;
        }
        unlockedNoteIds.remove(id);
        refreshSidebar();
        refreshNotesList(id);
        loadCurrentNote(id);
    }
}

void MainWindow::moveCurrentNoteToTrash()
{
    if (currentNoteId.isEmpty())
        return;

    const Note note = notesManager->note(currentNoteId);
    if (note.trashed) {
        purgeNote(currentNoteId);
        return;
    }

    if (QMessageBox::question(this, QStringLiteral("Move to Trash"),
                              QStringLiteral("Move \"%1\" to Recently Deleted?").arg(note.title))
        != QMessageBox::Yes) {
        return;
    }

    notesManager->deleteNote(currentNoteId);
    unlockedNoteIds.remove(currentNoteId);
    currentNoteId.clear();
    refreshSidebar();
    refreshNotesList();
}

void MainWindow::restoreNote(const QString &noteId)
{
    notesManager->restoreNote(noteId);
    refreshSidebar();
    refreshNotesList(noteId);
}

void MainWindow::purgeNote(const QString &noteId)
{
    const Note note = notesManager->note(noteId);
    if (QMessageBox::warning(this, QStringLiteral("Delete Permanently"),
                             QStringLiteral("Permanently delete \"%1\"?").arg(note.title),
                             QMessageBox::Yes | QMessageBox::Cancel)
        != QMessageBox::Yes) {
        return;
    }

    notesManager->purgeNote(noteId);
    unlockedNoteIds.remove(noteId);
    if (currentNoteId == noteId)
        currentNoteId.clear();
    refreshSidebar();
    refreshNotesList();
}

void MainWindow::exportNote(const QString &noteId)
{
    const QString id = noteId.isEmpty() ? currentNoteId : noteId;
    if (id.isEmpty() || !notesManager->hasNote(id))
        return;

    const Note note = notesManager->note(id);
    if (note.locked && !unlockedNoteIds.contains(id)) {
        handleLockAction(id);
        if (!unlockedNoteIds.contains(id))
            return;
    }

    const QString safeTitle = note.title.isEmpty() ? QStringLiteral("Untitled Note") : note.title;
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export Note"),
        safeTitle,
        QStringLiteral("PDF (*.pdf);;HTML (*.html);;Text (*.txt);;Markdown (*.md)"));
    if (path.isEmpty())
        return;

    if (path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(path);
        printer.setPageSize(QPageSize(QPageSize::A4));
        QTextDocument document;
        document.setHtml(note.html.isEmpty() ? note.body.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"))
                                             : note.html);
        document.print(&printer);
    } else {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("Export Failed"),
                                 QStringLiteral("Could not write the selected file."));
            return;
        }
        QTextStream stream(&file);
        if (path.endsWith(QStringLiteral(".html"), Qt::CaseInsensitive))
            stream << (note.html.isEmpty() ? note.body.toHtmlEscaped() : note.html);
        else
            stream << note.title << "\n\n" << note.body;
    }

    showStatusMessage(QStringLiteral("Exported"));
}

void MainWindow::importNote()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Import Note"),
        QString(),
        QStringLiteral("Notes (*.txt *.md *.html);;All Files (*.*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Import Failed"),
                             QStringLiteral("Could not read the selected file."));
        return;
    }

    const QString content = QString::fromUtf8(file.readAll());
    const QFileInfo info(path);
    const QString noteId = notesManager->createNote(info.completeBaseName(), content);
    if (info.suffix().compare(QStringLiteral("html"), Qt::CaseInsensitive) == 0) {
        QTextDocument document;
        document.setHtml(content);
        notesManager->updateNoteContent(noteId,
                                        info.completeBaseName(),
                                        document.toPlainText(),
                                        content,
                                        {});
    }

    currentFilterKind = QStringLiteral("all");
    currentFilterValue.clear();
    refreshSidebar();
    refreshNotesList(noteId);
    notesListWidget->selectNote(noteId);
}

void MainWindow::printNote(const QString &noteId)
{
    const QString id = noteId.isEmpty() ? currentNoteId : noteId;
    if (id.isEmpty() || !notesManager->hasNote(id))
        return;

    const Note note = notesManager->note(id);
    if (note.locked && !unlockedNoteIds.contains(id)) {
        handleLockAction(id);
        if (!unlockedNoteIds.contains(id))
            return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QTextDocument document;
    document.setHtml(note.html.isEmpty() ? note.body.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>"))
                                         : note.html);
    document.print(&printer);
}

void MainWindow::showSettingsDialog()
{
    SettingsDialog dialog(this);
    dialog.setSettings(appSettings);
    connect(&dialog, &SettingsDialog::settingsApplied,
            this, &MainWindow::applySettings);
    dialog.exec();
}

void MainWindow::showAccountMenu()
{
    QMenu menu(this);
    QAction *importAction = menu.addAction(QStringLiteral("Import Note"));
    QAction *exportAction = menu.addAction(QStringLiteral("Export Current Note"));
    QAction *printAction = menu.addAction(QStringLiteral("Print Current Note"));
    menu.addSeparator();
    QAction *storageAction = menu.addAction(QStringLiteral("Open Storage Folder"));
    menu.addSeparator();
    QAction *signInAction = menu.addAction(loginManager->stateDescription());
    QAction *signOutAction = menu.addAction(QStringLiteral("Sign Out"));
    signOutAction->setEnabled(loginManager->isSignedIn());

    QAction *chosen = menu.exec(btnAccount->mapToGlobal(QPoint(0, btnAccount->height())));
    if (chosen == importAction)
        importNote();
    else if (chosen == exportAction)
        exportNote(currentNoteId);
    else if (chosen == printAction)
        printNote(currentNoteId);
    else if (chosen == storageAction)
        QDesktopServices::openUrl(QUrl::fromLocalFile(notesManager->storageDirectory()));
    else if (chosen == signInAction) {
        loginManager->beginGoogleSignIn();
        showStatusMessage(loginManager->stateDescription());
    } else if (chosen == signOutAction) {
        loginManager->signOut();
        showStatusMessage(QStringLiteral("Signed out"));
    }
}

void MainWindow::showStatusMessage(const QString &message)
{
    QToolTip::showText(mapToGlobal(QPoint(width() / 2, 42)), message, this);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == titleSearchBox) {
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                titleSearchBox->clear();
                titleSearchBox->hide();
                btnSearch->show();
                refreshNotesList(currentNoteId);
                return true;
            }
        }
        if (event->type() == QEvent::FocusOut && titleSearchBox->text().trimmed().isEmpty()) {
            titleSearchBox->hide();
            btnSearch->show();
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const bool ctrl = event->modifiers() & Qt::ControlModifier;
    const bool shift = event->modifiers() & Qt::ShiftModifier;

    if (ctrl && !shift && event->key() == Qt::Key_F) {
        btnSearch->hide();
        titleSearchBox->show();
        titleSearchBox->setFocus();
        titleSearchBox->selectAll();
        event->accept();
        return;
    }

    if (ctrl && event->key() == Qt::Key_N) {
        createNewNote();
        event->accept();
        return;
    }

    if (ctrl && shift && event->key() == Qt::Key_N) {
        createFolder();
        event->accept();
        return;
    }

    if (ctrl && event->key() == Qt::Key_S) {
        notesManager->flushPendingChanges();
        showStatusMessage(QStringLiteral("Saved"));
        event->accept();
        return;
    }

    if (ctrl && event->key() == Qt::Key_L) {
        handleLockAction(currentNoteId);
        event->accept();
        return;
    }

    if (ctrl && event->key() == Qt::Key_E) {
        exportNote(currentNoteId);
        event->accept();
        return;
    }

    if (ctrl && event->key() == Qt::Key_P) {
        printNote(currentNoteId);
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape && titleSearchBox->isVisible()) {
        titleSearchBox->clear();
        titleSearchBox->hide();
        btnSearch->show();
        refreshNotesList(currentNoteId);
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
        && isTitleBarDragArea(this, titleBar, event->position().toPoint())) {
        dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && !dragPosition.isNull()) {
        QPoint globalPos = event->globalPosition().toPoint();
        if (isMaximized()) {
            const QPoint titlePos = titleBar ? titleBar->mapFromGlobal(globalPos) : QPoint(width() / 2, 22);
            const double ratio = titleBar && titleBar->width() > 0
                                     ? static_cast<double>(titlePos.x()) / static_cast<double>(titleBar->width())
                                     : 0.5;
            showNormal();
            const int offsetX = std::clamp(static_cast<int>(width() * ratio),
                                           80,
                                           std::max(80, width() - 80));
            dragPosition = QPoint(offsetX, 22);
            btnMax->setToolTip(QStringLiteral("Maximise"));
        }
        move(globalPos - dragPosition);
        event->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    dragPosition = QPoint();
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
        && isTitleBarDragArea(this, titleBar, event->position().toPoint())) {
        if (isMaximized()) {
            showNormal();
            btnMax->setToolTip(QStringLiteral("Maximise"));
        } else {
            showMaximized();
            btnMax->setToolTip(QStringLiteral("Restore"));
        }
        event->accept();
        return;
    }
    QMainWindow::mouseDoubleClickEvent(event);
}
