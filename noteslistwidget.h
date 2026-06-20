// noteslistwidget.h
#ifndef NOTESLISTWIDGET_H
#define NOTESLISTWIDGET_H

#include "notesmanager.h"

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QComboBox;

class NotesListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NotesListWidget(QWidget *parent = nullptr);

    QString currentNoteId() const;
    QString searchText() const;
    QString sortMode() const;
    void setHeader(const QString &title, int count);

public slots:
    void setNotes(const QList<Note> &notes, const QString &currentNoteId = QString());
    void selectNote(const QString &noteId);
    void focusSearch();
    void clearSearch();

signals:
    void noteSelected(const QString &noteId);
    void newNoteRequested();
    void searchChanged(const QString &query);
    void sortChanged(const QString &sortMode);
    void renameRequested(const QString &noteId);
    void duplicateRequested(const QString &noteId);
    void moveToFolderRequested(const QString &noteId);
    void pinRequested(const QString &noteId, bool pinned);
    void lockRequested(const QString &noteId);
    void moveToTrashRequested(const QString &noteId);
    void restoreRequested(const QString &noteId);
    void purgeRequested(const QString &noteId);
    void exportRequested(const QString &noteId);

private:
    void buildUI();
    void connectSignals();
    void rebuildList();
    void refreshSelection();
    void showContextMenu(const QPoint &position);
    QList<Note> sortedNotes() const;
    QListWidgetItem *appendNoteCard(const Note &note);

    QLabel *headerLabel = nullptr;
    QLabel *countLabel = nullptr;
    QLineEdit *notesSearchField = nullptr;
    QPushButton *newNoteButton = nullptr;
    QPushButton *viewToggleButton = nullptr;
    QComboBox *sortCombo = nullptr;
    QListWidget *notesList = nullptr;

    QList<Note> m_notes;
    QString m_currentNoteId;
    bool m_galleryMode = false;
};

#endif // NOTESLISTWIDGET_H
