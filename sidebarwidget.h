// sidebarwidget.h
#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include "notesmanager.h"

#include <QString>
#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;
class QPoint;

class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);

    QString currentKind() const;
    QString currentValue() const;

public slots:
    void setLibrary(const QList<NoteFolder> &folders,
                    const QList<SmartFolder> &smartFolders,
                    const QStringList &tags,
                    int allCount,
                    int pinnedCount,
                    int attachmentCount,
                    int lockedCount,
                    int trashCount);
    void selectFilter(const QString &kind, const QString &value = QString());

signals:
    void filterChanged(const QString &kind, const QString &value);
    void newFolderRequested();
    void newSmartFolderRequested();
    void renameFolderRequested(const QString &folderId);
    void deleteFolderRequested(const QString &folderId);
    void renameSmartFolderRequested(const QString &smartFolderId);
    void deleteSmartFolderRequested(const QString &smartFolderId);
    void renameTagRequested(const QString &tag);
    void deleteTagRequested(const QString &tag);

private:
    void buildUI();
    void connectSignals();
    void showContextMenu(const QPoint &position);
    void addHeader(const QString &title);
    void addFilterItem(const QString &title,
                       const QString &kind,
                       const QString &value = QString(),
                       int count = -1);

    QLabel *libraryLabel = nullptr;
    QListWidget *sidebarList = nullptr;
    QPushButton *newFolderButton = nullptr;
    QPushButton *smartFolderButton = nullptr;
};

#endif // SIDEBARWIDGET_H
