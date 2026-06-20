// sidebarwidget.cpp
#include "sidebarwidget.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr int KindRole = Qt::UserRole + 1;
constexpr int ValueRole = Qt::UserRole + 2;
constexpr int HeaderRole = Qt::UserRole + 3;

QPushButton *makeSidebarButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("sidebarActionButton"));
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}
}

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sidebarPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(220);
    setMaximumWidth(280);

    buildUI();
    connectSignals();
}

QString SidebarWidget::currentKind() const
{
    const QListWidgetItem *item = sidebarList ? sidebarList->currentItem() : nullptr;
    return item ? item->data(KindRole).toString() : QStringLiteral("all");
}

QString SidebarWidget::currentValue() const
{
    const QListWidgetItem *item = sidebarList ? sidebarList->currentItem() : nullptr;
    return item ? item->data(ValueRole).toString() : QString();
}

void SidebarWidget::setLibrary(const QList<NoteFolder> &folders,
                               const QList<SmartFolder> &smartFolders,
                               const QStringList &tags,
                               int allCount,
                               int pinnedCount,
                               int attachmentCount,
                               int lockedCount,
                               int trashCount)
{
    const QString previousKind = currentKind();
    const QString previousValue = currentValue();

    sidebarList->clear();
    addHeader(QStringLiteral("LIBRARY"));
    addFilterItem(QStringLiteral("All Notes"), QStringLiteral("all"), QString(), allCount);
    addFilterItem(QStringLiteral("Pinned"), QStringLiteral("pinned"), QString(), pinnedCount);
    addFilterItem(QStringLiteral("Recent"), QStringLiteral("recent"));
    addFilterItem(QStringLiteral("Attachments"), QStringLiteral("attachments"), QString(), attachmentCount);
    addFilterItem(QStringLiteral("All Images"), QStringLiteral("images"));
    addFilterItem(QStringLiteral("All PDFs"), QStringLiteral("pdfs"));
    addFilterItem(QStringLiteral("All Files"), QStringLiteral("files"));
    addFilterItem(QStringLiteral("Created Today"), QStringLiteral("created-today"));
    addFilterItem(QStringLiteral("Modified Today"), QStringLiteral("modified-today"));
    addFilterItem(QStringLiteral("Modified This Week"), QStringLiteral("modified-week"));
    addFilterItem(QStringLiteral("Untagged"), QStringLiteral("untagged"));
    addFilterItem(QStringLiteral("Locked Notes"), QStringLiteral("locked"), QString(), lockedCount);
    addFilterItem(QStringLiteral("Recently Deleted"), QStringLiteral("trash"), QString(), trashCount);

    addHeader(QStringLiteral("FOLDERS"));
    if (folders.isEmpty())
        addFilterItem(QStringLiteral("No Folders"), QStringLiteral("empty"), QString(), -1);
    for (const NoteFolder &folder : folders)
        addFilterItem(folder.name, QStringLiteral("folder"), folder.id);

    addHeader(QStringLiteral("SMART FOLDERS"));
    if (smartFolders.isEmpty())
        addFilterItem(QStringLiteral("No Smart Folders"), QStringLiteral("empty"), QString(), -1);
    for (const SmartFolder &folder : smartFolders)
        addFilterItem(folder.name, QStringLiteral("smart"), folder.id);

    addHeader(QStringLiteral("TAGS"));
    if (tags.isEmpty())
        addFilterItem(QStringLiteral("No Tags Yet"), QStringLiteral("empty"), QString(), -1);
    for (const QString &tag : tags)
        addFilterItem(QStringLiteral("#%1").arg(tag), QStringLiteral("tag"), tag);

    selectFilter(previousKind.isEmpty() ? QStringLiteral("all") : previousKind, previousValue);
    if (!sidebarList->currentItem())
        selectFilter(QStringLiteral("all"));
}

void SidebarWidget::selectFilter(const QString &kind, const QString &value)
{
    for (int row = 0; row < sidebarList->count(); ++row) {
        QListWidgetItem *item = sidebarList->item(row);
        if (item->data(KindRole).toString() == kind && item->data(ValueRole).toString() == value) {
            sidebarList->setCurrentItem(item);
            return;
        }
    }
}

void SidebarWidget::buildUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 14, 12, 12);
    layout->setSpacing(10);

    libraryLabel = new QLabel(QStringLiteral("1OS Notes"), this);
    libraryLabel->setObjectName(QStringLiteral("sidebarTitleLabel"));

    sidebarList = new QListWidget(this);
    sidebarList->setObjectName(QStringLiteral("sidebarList"));
    sidebarList->setFrameShape(QFrame::NoFrame);
    sidebarList->setSelectionMode(QAbstractItemView::SingleSelection);
    sidebarList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    sidebarList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarList->setContextMenuPolicy(Qt::CustomContextMenu);

    auto *buttonRow = new QWidget(this);
    auto *buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    newFolderButton = makeSidebarButton(QStringLiteral("Folder"), buttonRow);
    smartFolderButton = makeSidebarButton(QStringLiteral("Smart"), buttonRow);

    buttonLayout->addWidget(newFolderButton);
    buttonLayout->addWidget(smartFolderButton);

    layout->addWidget(libraryLabel);
    layout->addWidget(sidebarList, 1);
    layout->addWidget(buttonRow);
}

void SidebarWidget::connectSignals()
{
    connect(sidebarList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (!current || current->data(HeaderRole).toBool())
                    return;

                const QString kind = current->data(KindRole).toString();
                if (kind == QStringLiteral("empty"))
                    return;

                emit filterChanged(kind, current->data(ValueRole).toString());
            });

    connect(newFolderButton, &QPushButton::clicked, this, &SidebarWidget::newFolderRequested);
    connect(smartFolderButton, &QPushButton::clicked, this, &SidebarWidget::newSmartFolderRequested);
    connect(sidebarList, &QListWidget::customContextMenuRequested,
            this, &SidebarWidget::showContextMenu);
}

void SidebarWidget::showContextMenu(const QPoint &position)
{
    QListWidgetItem *item = sidebarList->itemAt(position);
    if (!item || item->data(HeaderRole).toBool())
        return;

    const QString kind = item->data(KindRole).toString();
    const QString value = item->data(ValueRole).toString();
    if (kind == QStringLiteral("empty") || value.isEmpty())
        return;

    QMenu menu(this);
    QAction *renameAction = nullptr;
    QAction *deleteAction = nullptr;

    if (kind == QStringLiteral("folder")) {
        renameAction = menu.addAction(QStringLiteral("Rename Folder"));
        deleteAction = menu.addAction(QStringLiteral("Delete Folder"));
    } else if (kind == QStringLiteral("smart")) {
        renameAction = menu.addAction(QStringLiteral("Rename Smart Folder"));
        deleteAction = menu.addAction(QStringLiteral("Delete Smart Folder"));
    } else if (kind == QStringLiteral("tag")) {
        renameAction = menu.addAction(QStringLiteral("Rename Tag"));
        deleteAction = menu.addAction(QStringLiteral("Delete Tag"));
    } else {
        return;
    }

    QAction *chosen = menu.exec(sidebarList->mapToGlobal(position));
    if (!chosen)
        return;

    if (kind == QStringLiteral("folder")) {
        if (chosen == renameAction)
            emit renameFolderRequested(value);
        else if (chosen == deleteAction)
            emit deleteFolderRequested(value);
    } else if (kind == QStringLiteral("smart")) {
        if (chosen == renameAction)
            emit renameSmartFolderRequested(value);
        else if (chosen == deleteAction)
            emit deleteSmartFolderRequested(value);
    } else if (kind == QStringLiteral("tag")) {
        if (chosen == renameAction)
            emit renameTagRequested(value);
        else if (chosen == deleteAction)
            emit deleteTagRequested(value);
    }
}

void SidebarWidget::addHeader(const QString &title)
{
    auto *item = new QListWidgetItem(title, sidebarList);
    item->setFlags(Qt::NoItemFlags);
    item->setData(HeaderRole, true);
    item->setSizeHint(QSize(0, 28));
}

void SidebarWidget::addFilterItem(const QString &title,
                                  const QString &kind,
                                  const QString &value,
                                  int count)
{
    QString label = title;
    if (count >= 0)
        label = QStringLiteral("%1  %2").arg(title).arg(count);

    auto *item = new QListWidgetItem(label, sidebarList);
    item->setSizeHint(QSize(0, 36));
    item->setData(KindRole, kind);
    item->setData(ValueRole, value);
    item->setData(HeaderRole, false);

    if (kind == QStringLiteral("empty"))
        item->setFlags(Qt::NoItemFlags);
}
