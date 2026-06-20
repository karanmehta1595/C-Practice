// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "notesmanager.h"
#include "settingsdialog.h"

#include <QImage>
#include <QMainWindow>
#include <QPoint>
#include <QSet>
#include <QStringList>

class QLabel;
class QPushButton;
class QLineEdit;
class QKeyEvent;
class QMouseEvent;
class QWidget;

class LoginManager;
class SidebarWidget;
class NotesListWidget;
class NotesEditorWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum ThemeMode {
        SystemTheme,
        DarkTheme,
        LightTheme
    };

    void buildUI();
    void buildTitleBar();
    void connectSignals();
    void applyCurrentTheme();
    void setTheme(ThemeMode theme);
    void applySettings(const AppSettings &settings);
    void ensureStarterContent();
    void refreshSidebar();
    void refreshNotesList(const QString &preferredNoteId = QString());
    void loadCurrentNote(const QString &noteId);
    QList<Note> notesForCurrentFilter() const;
    QString currentFilterTitle() const;
    bool currentNoteIsLockedAndHidden() const;

    void createNewNote();
    void createNoteFromTemplate(const QString &templateName);
    void renameNote(const QString &noteId);
    void duplicateNote(const QString &noteId);
    void createFolder();
    void createSmartFolder();
    void renameFolder(const QString &folderId);
    void deleteFolder(const QString &folderId);
    void renameSmartFolder(const QString &smartFolderId);
    void deleteSmartFolder(const QString &smartFolderId);
    void renameTag(const QString &tag);
    void deleteTag(const QString &tag);
    void moveNoteToFolder(const QString &noteId);
    void attachFileToCurrentNote();
    void attachFilesToCurrentNote(const QStringList &filePaths);
    void attachPastedImageToCurrentNote(const QImage &image);
    void handleLockAction(const QString &noteId = QString());
    void moveCurrentNoteToTrash();
    void restoreNote(const QString &noteId);
    void purgeNote(const QString &noteId);
    void emptyTrash();
    void exportNote(const QString &noteId = QString());
    void importNote();
    void printNote(const QString &noteId = QString());
    void openLinkedNote(const QString &title);
    void showSettingsDialog();
    void showAccountMenu();
    void showStatusMessage(const QString &message);

    QWidget *centralWidget = nullptr;
    QWidget *titleBar = nullptr;
    QLabel *titleLabel = nullptr;

    QPushButton *btnClose = nullptr;
    QPushButton *btnMin = nullptr;
    QPushButton *btnMax = nullptr;
    QPushButton *btnSearch = nullptr;
    QPushButton *btnSettings = nullptr;
    QPushButton *btnAccount = nullptr;
    QLineEdit *titleSearchBox = nullptr;

    SidebarWidget *sidebarWidget = nullptr;
    NotesListWidget *notesListWidget = nullptr;
    NotesEditorWidget *notesEditorWidget = nullptr;

    NotesManager *notesManager = nullptr;
    LoginManager *loginManager = nullptr;

    ThemeMode currentTheme = DarkTheme;
    AppSettings appSettings;
    QString currentFilterKind = QStringLiteral("all");
    QString currentFilterValue;
    QString currentNoteId;
    QSet<QString> unlockedNoteIds;
    QPoint dragPosition;
};

#endif // MAINWINDOW_H
