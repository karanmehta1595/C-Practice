// noteslistwidget.cpp
#include "noteslistwidget.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int NoteIdRole = Qt::UserRole + 1;

QPushButton *makeButton(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

QString previewFromNote(const Note &note)
{
    QString preview = note.locked ? QStringLiteral("Locked note") : note.body.simplified();
    if (preview.isEmpty())
        preview = QStringLiteral("No additional text");
    if (preview.size() > 120)
        preview = preview.left(120).trimmed() + QStringLiteral("...");
    return preview;
}

QString relativeDate(const QDateTime &dateTime)
{
    if (!dateTime.isValid())
        return QStringLiteral("Unknown");

    const QDate date = dateTime.toLocalTime().date();
    const QDate today = QDate::currentDate();
    if (date == today)
        return QStringLiteral("Today");
    if (date == today.addDays(-1))
        return QStringLiteral("Yesterday");
    if (date.year() == today.year())
        return date.toString(QStringLiteral("MMM d"));
    return date.toString(QStringLiteral("MMM d, yyyy"));
}

QString metaLine(const Note &note)
{
    QStringList parts;
    parts << relativeDate(note.updatedAt);
    if (note.pinned)
        parts << QStringLiteral("Pinned");
    if (note.locked)
        parts << QStringLiteral("Locked");
    if (!note.attachments.isEmpty())
        parts << QStringLiteral("%1 attachment%2")
                     .arg(note.attachments.size())
                     .arg(note.attachments.size() == 1 ? QString() : QStringLiteral("s"));
    return parts.join(QStringLiteral(" - "));
}

QString tagsLine(const Note &note)
{
    if (note.tags.isEmpty())
        return QString();

    QStringList tags;
    for (const QString &tag : note.tags)
        tags << QStringLiteral("#%1").arg(tag);
    return tags.join(QStringLiteral(" "));
}

void repolish(QWidget *widget)
{
    if (!widget)
        return;
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QWidget *createNoteCardWidget(const Note &note, QWidget *parent)
{
    auto *card = new QWidget(parent);
    card->setObjectName(QStringLiteral("noteCard"));
    card->setProperty("selected", false);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(5);

    auto *topRow = new QWidget(card);
    auto *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(6);

    auto *titleLabel = new QLabel(note.title, topRow);
    titleLabel->setObjectName(QStringLiteral("noteTitleLabel"));
    titleLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    titleLabel->setWordWrap(false);

    auto *stateLabel = new QLabel(topRow);
    stateLabel->setObjectName(QStringLiteral("noteStateLabel"));
    QStringList states;
    if (note.pinned)
        states << QStringLiteral("PIN");
    if (note.locked)
        states << QStringLiteral("LOCK");
    stateLabel->setText(states.join(QStringLiteral(" ")));

    topLayout->addWidget(titleLabel, 1);
    topLayout->addWidget(stateLabel);

    auto *previewLabel = new QLabel(previewFromNote(note), card);
    previewLabel->setObjectName(QStringLiteral("notePreviewLabel"));
    previewLabel->setWordWrap(true);
    previewLabel->setMaximumHeight(42);
    previewLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    auto *dateLabel = new QLabel(metaLine(note), card);
    dateLabel->setObjectName(QStringLiteral("noteDateLabel"));
    dateLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    auto *tagsLabel = new QLabel(tagsLine(note), card);
    tagsLabel->setObjectName(QStringLiteral("noteTagsLabel"));
    tagsLabel->setWordWrap(true);
    tagsLabel->setVisible(!note.tags.isEmpty());

    layout->addWidget(topRow);
    layout->addWidget(previewLabel);
    layout->addStretch();
    layout->addWidget(tagsLabel);
    layout->addWidget(dateLabel);

    return card;
}
}

NotesListWidget::NotesListWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("notesPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(280);
    setMaximumWidth(380);

    buildUI();
    connectSignals();
}

QString NotesListWidget::currentNoteId() const
{
    return m_currentNoteId;
}

QString NotesListWidget::searchText() const
{
    return notesSearchField ? notesSearchField->text().trimmed() : QString();
}

QString NotesListWidget::sortMode() const
{
    return sortCombo ? sortCombo->currentData().toString() : QStringLiteral("updated");
}

void NotesListWidget::setHeader(const QString &title, int count)
{
    headerLabel->setText(title);
    countLabel->setText(QStringLiteral("%1 note%2").arg(count).arg(count == 1 ? QString() : QStringLiteral("s")));
}

void NotesListWidget::setNotes(const QList<Note> &notes, const QString &currentNoteId)
{
    m_notes = notes;
    if (!currentNoteId.isEmpty())
        m_currentNoteId = currentNoteId;
    rebuildList();
}

void NotesListWidget::selectNote(const QString &noteId)
{
    if (noteId.isEmpty())
        return;

    for (int row = 0; row < notesList->count(); ++row) {
        QListWidgetItem *item = notesList->item(row);
        if (item->data(NoteIdRole).toString() == noteId) {
            notesList->setCurrentItem(item);
            return;
        }
    }
}

void NotesListWidget::focusSearch()
{
    notesSearchField->setFocus();
    notesSearchField->selectAll();
}

void NotesListWidget::clearSearch()
{
    notesSearchField->clear();
}

void NotesListWidget::buildUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto *titleRow = new QWidget(this);
    auto *titleLayout = new QHBoxLayout(titleRow);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(8);

    headerLabel = new QLabel(QStringLiteral("All Notes"), titleRow);
    headerLabel->setObjectName(QStringLiteral("notesHeaderTitle"));

    countLabel = new QLabel(QStringLiteral("0 notes"), titleRow);
    countLabel->setObjectName(QStringLiteral("notesMetaLabel"));

    newNoteButton = makeButton(QStringLiteral("New"), QStringLiteral("primaryActionButton"), titleRow);
    newNoteButton->setToolTip(QStringLiteral("New Note"));

    titleLayout->addWidget(headerLabel, 1);
    titleLayout->addWidget(newNoteButton);

    notesSearchField = new QLineEdit(this);
    notesSearchField->setObjectName(QStringLiteral("notesSearchField"));
    notesSearchField->setPlaceholderText(QStringLiteral("Search notes"));
    notesSearchField->setClearButtonEnabled(true);

    auto *controlRow = new QWidget(this);
    auto *controlLayout = new QHBoxLayout(controlRow);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(8);

    sortCombo = new QComboBox(controlRow);
    sortCombo->setObjectName(QStringLiteral("sortCombo"));
    sortCombo->addItem(QStringLiteral("Edited"), QStringLiteral("updated"));
    sortCombo->addItem(QStringLiteral("Created"), QStringLiteral("created"));
    sortCombo->addItem(QStringLiteral("Title"), QStringLiteral("title"));

    viewToggleButton = makeButton(QStringLiteral("Gallery"), QStringLiteral("secondaryActionButton"), controlRow);
    viewToggleButton->setToolTip(QStringLiteral("Toggle list/gallery view"));

    controlLayout->addWidget(countLabel, 1);
    controlLayout->addWidget(sortCombo);
    controlLayout->addWidget(viewToggleButton);

    notesList = new QListWidget(this);
    notesList->setObjectName(QStringLiteral("notesList"));
    notesList->setFrameShape(QFrame::NoFrame);
    notesList->setSelectionMode(QAbstractItemView::SingleSelection);
    notesList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    notesList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    notesList->setContextMenuPolicy(Qt::CustomContextMenu);
    notesList->setSpacing(8);

    layout->addWidget(titleRow);
    layout->addWidget(notesSearchField);
    layout->addWidget(controlRow);
    layout->addWidget(notesList, 1);
}

void NotesListWidget::connectSignals()
{
    connect(newNoteButton, &QPushButton::clicked, this, &NotesListWidget::newNoteRequested);
    connect(notesSearchField, &QLineEdit::textChanged, this, &NotesListWidget::searchChanged);

    connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        rebuildList();
        emit sortChanged(sortMode());
    });

    connect(viewToggleButton, &QPushButton::clicked, this, [this]() {
        m_galleryMode = !m_galleryMode;
        notesList->setViewMode(m_galleryMode ? QListView::IconMode : QListView::ListMode);
        notesList->setResizeMode(QListView::Adjust);
        notesList->setMovement(QListView::Static);
        notesList->setWrapping(m_galleryMode);
        viewToggleButton->setText(m_galleryMode ? QStringLiteral("List") : QStringLiteral("Gallery"));
        rebuildList();
    });

    connect(notesList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (!current)
                    return;
                m_currentNoteId = current->data(NoteIdRole).toString();
                emit noteSelected(m_currentNoteId);
                refreshSelection();
            });

    connect(notesList, &QListWidget::customContextMenuRequested,
            this, &NotesListWidget::showContextMenu);
}

void NotesListWidget::rebuildList()
{
    const QString selectedId = m_currentNoteId;
    notesList->clear();

    const QList<Note> notes = sortedNotes();
    for (const Note &note : notes)
        appendNoteCard(note);

    if (!selectedId.isEmpty())
        selectNote(selectedId);
    if (!notesList->currentItem() && notesList->count() > 0)
        notesList->setCurrentRow(0);
    refreshSelection();
}

void NotesListWidget::refreshSelection()
{
    for (int row = 0; row < notesList->count(); ++row) {
        QListWidgetItem *item = notesList->item(row);
        QWidget *card = notesList->itemWidget(item);
        if (!card)
            continue;
        card->setProperty("selected", item == notesList->currentItem());
        repolish(card);
    }
}

void NotesListWidget::showContextMenu(const QPoint &position)
{
    QListWidgetItem *item = notesList->itemAt(position);
    if (!item)
        return;

    const QString noteId = item->data(NoteIdRole).toString();
    auto it = std::find_if(m_notes.begin(), m_notes.end(), [&noteId](const Note &note) {
        return note.id == noteId;
    });
    if (it == m_notes.end())
        return;

    const Note note = *it;
    QMenu menu(this);

    QAction *renameAction = nullptr;
    QAction *duplicateAction = nullptr;
    QAction *moveAction = nullptr;
    if (!note.trashed) {
        renameAction = menu.addAction(QStringLiteral("Rename Note"));
        duplicateAction = menu.addAction(QStringLiteral("Duplicate Note"));
        moveAction = menu.addAction(QStringLiteral("Move to Folder"));
        menu.addSeparator();
    }

    QAction *pinAction = menu.addAction(note.pinned ? QStringLiteral("Unpin Note")
                                                    : QStringLiteral("Pin Note"));
    pinAction->setEnabled(!note.trashed);
    QAction *lockAction = menu.addAction(note.locked ? QStringLiteral("Unlock or Remove Lock")
                                                     : QStringLiteral("Lock Note"));
    lockAction->setEnabled(!note.trashed);
    QAction *exportAction = menu.addAction(QStringLiteral("Export Note"));
    exportAction->setEnabled(!note.trashed);
    menu.addSeparator();
    QAction *deleteAction = nullptr;
    QAction *restoreAction = nullptr;
    QAction *purgeAction = nullptr;
    if (note.trashed) {
        restoreAction = menu.addAction(QStringLiteral("Restore Note"));
        purgeAction = menu.addAction(QStringLiteral("Delete Permanently"));
    } else {
        deleteAction = menu.addAction(QStringLiteral("Move to Trash"));
    }

    QAction *chosen = menu.exec(notesList->mapToGlobal(position));
    if (!chosen)
        return;

    if (chosen == renameAction)
        emit renameRequested(noteId);
    else if (chosen == duplicateAction)
        emit duplicateRequested(noteId);
    else if (chosen == moveAction)
        emit moveToFolderRequested(noteId);
    else if (chosen == pinAction)
        emit pinRequested(noteId, !note.pinned);
    else if (chosen == lockAction)
        emit lockRequested(noteId);
    else if (chosen == exportAction)
        emit exportRequested(noteId);
    else if (chosen == deleteAction)
        emit moveToTrashRequested(noteId);
    else if (chosen == restoreAction)
        emit restoreRequested(noteId);
    else if (chosen == purgeAction)
        emit purgeRequested(noteId);
}

QList<Note> NotesListWidget::sortedNotes() const
{
    QList<Note> notes = m_notes;
    const QString mode = sortMode();
    std::sort(notes.begin(), notes.end(), [&mode](const Note &left, const Note &right) {
        if (left.pinned != right.pinned)
            return left.pinned;
        if (mode == QStringLiteral("title"))
            return left.title.compare(right.title, Qt::CaseInsensitive) < 0;
        if (mode == QStringLiteral("created"))
            return left.createdAt > right.createdAt;
        return left.updatedAt > right.updatedAt;
    });
    return notes;
}

QListWidgetItem *NotesListWidget::appendNoteCard(const Note &note)
{
    auto *item = new QListWidgetItem(notesList);
    item->setData(NoteIdRole, note.id);
    item->setSizeHint(m_galleryMode ? QSize(156, 146) : QSize(0, 116));

    auto *card = createNoteCardWidget(note, notesList);
    notesList->setItemWidget(item, card);
    return item;
}
