// noteseditorwidget.cpp
#include "noteseditorwidget.h"

#include "notelinkservice.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSizeGrip>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextListFormat>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {
QPushButton *makeToolButton(const QString &text, const QString &tooltip, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("formatButton"));
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tooltip);
    return button;
}

QStringList extractTags(const QString &text)
{
    QStringList tags;
    QRegularExpression regex(QStringLiteral(R"((?:^|\s)#([A-Za-z0-9_-]+))"));
    QRegularExpressionMatchIterator it = regex.globalMatch(text);
    while (it.hasNext()) {
        const QString tag = it.next().captured(1).trimmed();
        if (!tag.isEmpty() && !tags.contains(tag, Qt::CaseInsensitive))
            tags.append(tag);
    }
    tags.sort(Qt::CaseInsensitive);
    return tags;
}

int wordCount(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return 0;
    return trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).count();
}
}

NotesEditorWidget::NotesEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("editorPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(520);

    buildUI();
    connectSignals();
    updateDocumentStats();
}

QString NotesEditorWidget::title() const
{
    return titleEdit ? titleEdit->text() : QString();
}

QString NotesEditorWidget::plainText() const
{
    return editor ? editor->toPlainText() : QString();
}

QString NotesEditorWidget::html() const
{
    return editor ? editor->toHtml() : QString();
}

QStringList NotesEditorWidget::detectedTags() const
{
    return extractTags(title() + QLatin1Char('\n') + plainText());
}

void NotesEditorWidget::setEditorFontSize(int pointSize)
{
    if (!editor)
        return;
    QFont font = editor->font();
    font.setPointSize(pointSize);
    editor->setFont(font);
}

void NotesEditorWidget::setAutosaveText(const QString &text)
{
    if (autosavedLabel)
        autosavedLabel->setText(text);
}

void NotesEditorWidget::loadNote(const Note &note, bool locked)
{
    m_loading = true;
    m_currentNote = note;
    m_lockedView = locked;

    const QSignalBlocker titleBlocker(titleEdit);
    const QSignalBlocker editorBlocker(editor);

    titleEdit->setText(note.title);
    if (locked) {
        editor->setPlainText(note.passwordHint.isEmpty()
                                 ? QStringLiteral("This note is locked.")
                                 : QStringLiteral("This note is locked.\n\nHint: %1").arg(note.passwordHint));
    } else if (!note.html.isEmpty()) {
        editor->setHtml(note.html);
    } else {
        editor->setPlainText(note.body);
    }

    setEditorEnabledForLock(locked);
    updateAttachmentSummary();
    updateDocumentStats();
    if (dateLabel) {
        const QString created = note.createdAt.isValid()
                                    ? note.createdAt.toLocalTime().toString(QStringLiteral("MMM d, yyyy h:mm AP"))
                                    : QStringLiteral("Unknown");
        const QString edited = note.updatedAt.isValid()
                                   ? note.updatedAt.toLocalTime().toString(QStringLiteral("MMM d, yyyy h:mm AP"))
                                   : QStringLiteral("Unknown");
        dateLabel->setText(QStringLiteral("Created %1  Edited %2").arg(created, edited));
    }
    setAutosaveText(QStringLiteral("Autosaved"));
    updateCalculationSuggestion();
    m_loading = false;
}

void NotesEditorWidget::loadNote(const QString &title, const QString &body)
{
    Note note;
    note.title = title;
    note.body = body;
    loadNote(note, false);
}

void NotesEditorWidget::clearEditor()
{
    m_loading = true;
    m_currentNote = Note();
    titleEdit->clear();
    editor->clear();
    attachmentsLabel->clear();
    if (dateLabel)
        dateLabel->clear();
    if (calculatorSuggestionLabel)
        calculatorSuggestionLabel->hide();
    setEditorEnabledForLock(false);
    updateDocumentStats();
    m_loading = false;
}

void NotesEditorWidget::focusTitleAndSelectAll()
{
    titleEdit->setFocus();
    titleEdit->selectAll();
}

void NotesEditorWidget::insertAttachmentReference(const NoteAttachment &attachment)
{
    if (!editor || attachment.displayName.isEmpty())
        return;

    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (!editor->toPlainText().endsWith(QLatin1Char('\n')))
        cursor.insertBlock();
    cursor.insertHtml(QStringLiteral("<p><b>Attachment:</b> <a href=\"file:///%1\">%2</a></p>")
                          .arg(QString(attachment.filePath).replace(QLatin1Char('\\'), QLatin1Char('/')),
                               attachment.displayName.toHtmlEscaped()));
    editor->setTextCursor(cursor);
    emitContentChanged();
}

void NotesEditorWidget::buildUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 8, 22, 8);
    layout->setSpacing(8);

    buildToolbar();

    titleEdit = new QLineEdit(this);
    titleEdit->setObjectName(QStringLiteral("noteTitleField"));
    titleEdit->setPlaceholderText(QStringLiteral("Untitled Note"));
    titleEdit->setClearButtonEnabled(false);

    attachmentsLabel = new QLabel(this);
    attachmentsLabel->setObjectName(QStringLiteral("attachmentsLabel"));
    attachmentsLabel->setWordWrap(true);

    editor = new QTextEdit(this);
    editor->setObjectName(QStringLiteral("editor"));
    editor->setAcceptRichText(true);
    editor->setPlaceholderText(QStringLiteral("Start writing..."));
    editor->setTabChangesFocus(false);
    editor->setAcceptDrops(true);
    editor->installEventFilter(this);
    //editor->setOpenLinks(false);
    editor->setFont(QFont(QStringLiteral("Segoe UI"), 12));

    buildStatusBar();

    layout->addWidget(editorToolbar);
    layout->addWidget(titleEdit);
    layout->addWidget(attachmentsLabel);
    layout->addWidget(editor, 1);
    layout->addWidget(statusBarPanel);
}

void NotesEditorWidget::buildToolbar()
{
    editorToolbar = new QWidget(this);
    editorToolbar->setObjectName(QStringLiteral("editorToolbar"));
    editorToolbar->setFixedHeight(40);

    auto *toolbarLayout = new QHBoxLayout(editorToolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(7);

    styleCombo = new QComboBox(editorToolbar);
    styleCombo->setObjectName(QStringLiteral("styleCombo"));
    styleCombo->addItem(QStringLiteral("Body"), QStringLiteral("body"));
    styleCombo->addItem(QStringLiteral("Title"), QStringLiteral("title"));
    styleCombo->addItem(QStringLiteral("Heading"), QStringLiteral("heading"));
    styleCombo->addItem(QStringLiteral("Subheading"), QStringLiteral("subheading"));
    styleCombo->addItem(QStringLiteral("Monospace"), QStringLiteral("mono"));

    btnBold = makeToolButton(QStringLiteral("B"), QStringLiteral("Bold"), editorToolbar);
    btnItalic = makeToolButton(QStringLiteral("I"), QStringLiteral("Italic"), editorToolbar);
    btnUnderline = makeToolButton(QStringLiteral("U"), QStringLiteral("Underline"), editorToolbar);
    btnStrike = makeToolButton(QStringLiteral("S"), QStringLiteral("Strikethrough"), editorToolbar);
    btnHighlight = makeToolButton(QStringLiteral("HL"), QStringLiteral("Highlight"), editorToolbar);
    btnBulletList = makeToolButton(QStringLiteral("List"), QStringLiteral("Bulleted List"), editorToolbar);
    btnNumberedList = makeToolButton(QStringLiteral("1."), QStringLiteral("Numbered List"), editorToolbar);
    btnChecklist = makeToolButton(QStringLiteral("Check"), QStringLiteral("Checklist"), editorToolbar);
    btnTable = makeToolButton(QStringLiteral("Table"), QStringLiteral("Insert Table"), editorToolbar);
    btnLink = makeToolButton(QStringLiteral("Link"), QStringLiteral("Insert Link"), editorToolbar);
    btnAttach = makeToolButton(QStringLiteral("Attach"), QStringLiteral("Attach File"), editorToolbar);
    btnLock = makeToolButton(QStringLiteral("Lock"), QStringLiteral("Lock or Unlock Note"), editorToolbar);
    btnExport = makeToolButton(QStringLiteral("Export"), QStringLiteral("Export Note"), editorToolbar);
    btnPrint = makeToolButton(QStringLiteral("Print"), QStringLiteral("Print Note"), editorToolbar);
    btnDelete = makeToolButton(QStringLiteral("Trash"), QStringLiteral("Move Note to Trash"), editorToolbar);
    btnDelete->setObjectName(QStringLiteral("dangerButton"));

    toolbarLayout->addWidget(styleCombo);
    toolbarLayout->addWidget(btnBold);
    toolbarLayout->addWidget(btnItalic);
    toolbarLayout->addWidget(btnUnderline);
    toolbarLayout->addWidget(btnStrike);
    toolbarLayout->addWidget(btnHighlight);
    toolbarLayout->addSpacing(4);
    toolbarLayout->addWidget(btnBulletList);
    toolbarLayout->addWidget(btnNumberedList);
    toolbarLayout->addWidget(btnChecklist);
    toolbarLayout->addWidget(btnTable);
    toolbarLayout->addWidget(btnLink);
    toolbarLayout->addWidget(btnAttach);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(btnLock);
    toolbarLayout->addWidget(btnExport);
    toolbarLayout->addWidget(btnPrint);
    toolbarLayout->addWidget(btnDelete);
}

void NotesEditorWidget::buildStatusBar()
{
    statusBarPanel = new QWidget(this);
    statusBarPanel->setObjectName(QStringLiteral("statusBarPanel"));
    statusBarPanel->setFixedHeight(32);

    auto *layout = new QHBoxLayout(statusBarPanel);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(16);

    wordCountLabel = new QLabel(QStringLiteral("0 Words"), statusBarPanel);
    wordCountLabel->setObjectName(QStringLiteral("statusLabel"));
    characterCountLabel = new QLabel(QStringLiteral("0 Characters"), statusBarPanel);
    characterCountLabel->setObjectName(QStringLiteral("statusLabel"));
    readingTimeLabel = new QLabel(QStringLiteral("0 min"), statusBarPanel);
    readingTimeLabel->setObjectName(QStringLiteral("statusLabel"));
    checklistProgressLabel = new QLabel(statusBarPanel);
    checklistProgressLabel->setObjectName(QStringLiteral("statusLabel"));
    checklistProgressLabel->hide();
    dateLabel = new QLabel(statusBarPanel);
    dateLabel->setObjectName(QStringLiteral("statusLabel"));
    dateLabel->setMinimumWidth(190);
    tagCountLabel = new QLabel(QStringLiteral("0 Tags"), statusBarPanel);
    tagCountLabel->setObjectName(QStringLiteral("statusLabel"));
    calculatorSuggestionLabel = new QLabel(statusBarPanel);
    calculatorSuggestionLabel->setObjectName(QStringLiteral("statusLabel"));
    calculatorSuggestionLabel->setMaximumWidth(260);
    calculatorSuggestionLabel->hide();
    autosavedLabel = new QLabel(QStringLiteral("Autosaved"), statusBarPanel);
    autosavedLabel->setObjectName(QStringLiteral("statusLabel"));

    auto *grip = new QSizeGrip(statusBarPanel);

    layout->addWidget(wordCountLabel);
    layout->addWidget(characterCountLabel);
    layout->addWidget(readingTimeLabel);
    layout->addWidget(checklistProgressLabel);
    layout->addWidget(tagCountLabel);
    layout->addWidget(dateLabel);
    layout->addStretch();
    layout->addWidget(calculatorSuggestionLabel);
    layout->addWidget(autosavedLabel);
    layout->addWidget(grip, 0, Qt::AlignRight | Qt::AlignBottom);
}

void NotesEditorWidget::connectSignals()
{
    connect(titleEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (m_loading || m_markdownFormatting)
            return;
        emit titleChanged(text);
        emitContentChanged();
    });

    connect(editor, &QTextEdit::textChanged, this, [this]() {
        if (m_loading)
            return;
        applyMarkdownShortcut();
        updateChecklistFormatting();
        updateDocumentStats();
        updateCalculationSuggestion();
        emit bodyChanged(editor->toPlainText());
        emitContentChanged();
        setAutosaveText(QStringLiteral("Autosaving..."));
        QPointer<QLabel> autosaved = autosavedLabel;
        QTimer::singleShot(650, this, [autosaved]() {
            if (autosaved)
                autosaved->setText(QStringLiteral("Autosaved"));
        });
    });

    connect(styleCombo, &QComboBox::currentTextChanged, this, [this]() {
        applyBlockStyle(styleCombo->currentData().toString());
    });

    connect(btnBold, &QPushButton::clicked, this, [this]() {
        QTextCharFormat format;
        format.setFontWeight(editor->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        mergeFormat(format);
    });
    connect(btnItalic, &QPushButton::clicked, this, [this]() {
        QTextCharFormat format;
        format.setFontItalic(!editor->fontItalic());
        mergeFormat(format);
    });
    connect(btnUnderline, &QPushButton::clicked, this, [this]() {
        QTextCharFormat format;
        format.setFontUnderline(!editor->fontUnderline());
        mergeFormat(format);
    });
    connect(btnStrike, &QPushButton::clicked, this, [this]() {
        QTextCharFormat format;
        format.setFontStrikeOut(!editor->currentCharFormat().fontStrikeOut());
        mergeFormat(format);
    });
    connect(btnHighlight, &QPushButton::clicked, this, [this]() {
        QTextCharFormat format;
        format.setBackground(QColor(QStringLiteral("#FFE08A")));
        mergeFormat(format);
    });
    connect(btnBulletList, &QPushButton::clicked, this, [this]() {
        QTextCursor cursor = editor->textCursor();
        QTextListFormat format;
        format.setStyle(QTextListFormat::ListDisc);
        cursor.createList(format);
        editor->setFocus();
    });
    connect(btnNumberedList, &QPushButton::clicked, this, [this]() {
        QTextCursor cursor = editor->textCursor();
        QTextListFormat format;
        format.setStyle(QTextListFormat::ListDecimal);
        cursor.createList(format);
        editor->setFocus();
    });
    connect(btnChecklist, &QPushButton::clicked, this, [this]() {
        QTextCursor cursor = editor->textCursor();
        cursor.beginEditBlock();
        if (!cursor.atBlockStart())
            cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.insertBlock();
        cursor.insertText(QStringLiteral("- [ ] "));
        cursor.endEditBlock();
        editor->setTextCursor(cursor);
        editor->setFocus();
    });
    connect(btnTable, &QPushButton::clicked, this, [this]() {
        QTextCursor cursor = editor->textCursor();
        cursor.insertHtml(QStringLiteral(
            "<table border=\"1\" cellspacing=\"0\" cellpadding=\"6\">"
            "<tr><td><b>Item</b></td><td><b>Notes</b></td></tr>"
            "<tr><td></td><td></td></tr>"
            "</table><p></p>"));
        editor->setFocus();
    });
    connect(btnLink, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString url = QInputDialog::getText(this, QStringLiteral("Insert Link"),
                                                  QStringLiteral("URL:"), QLineEdit::Normal,
                                                  QStringLiteral("https://"), &ok);
        if (!ok || url.trimmed().isEmpty())
            return;
        QTextCharFormat format;
        format.setAnchor(true);
        format.setAnchorHref(url.trimmed());
        format.setForeground(QColor(QStringLiteral("#1D5FD1")));
        format.setFontUnderline(true);
        mergeFormat(format);
    });

    connect(btnAttach, &QPushButton::clicked, this, &NotesEditorWidget::attachRequested);
    connect(btnLock, &QPushButton::clicked, this, &NotesEditorWidget::lockRequested);
    connect(btnDelete, &QPushButton::clicked, this, &NotesEditorWidget::deleteRequested);
    connect(btnExport, &QPushButton::clicked, this, &NotesEditorWidget::exportRequested);
    connect(btnPrint, &QPushButton::clicked, this, &NotesEditorWidget::printRequested);
}

void NotesEditorWidget::emitContentChanged()
{
    updateDocumentStats();
    emit contentChanged(title(), plainText(), html(), detectedTags());
}

void NotesEditorWidget::updateDocumentStats()
{
    const QString text = plainText();
    const NoteMetadata metadata = NoteModel::metadataForText(text);
    wordCountLabel->setText(QStringLiteral("%1 Words").arg(metadata.wordCount));
    characterCountLabel->setText(QStringLiteral("%1 Characters").arg(text.size()));
    if (readingTimeLabel)
        readingTimeLabel->setText(QStringLiteral("%1 min read").arg(metadata.readingTimeMinutes));
    tagCountLabel->setText(QStringLiteral("%1 Tags").arg(detectedTags().size()));

    QRegularExpression checkboxRegex(QStringLiteral(R"((?:^|\n)\s*(?:-\s*)?\[([ xX])\])"));
    QRegularExpressionMatchIterator it = checkboxRegex.globalMatch(text);
    int total = 0;
    int completed = 0;
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        ++total;
        const QString state = match.captured(1);
        if (state.compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0)
            ++completed;
    }
    if (checklistProgressLabel) {
        checklistProgressLabel->setVisible(total > 0);
        checklistProgressLabel->setText(QStringLiteral("%1 / %2 Done").arg(completed).arg(total));
    }
}

void NotesEditorWidget::updateCalculationSuggestion()
{
    if (!calculatorSuggestionLabel || m_lockedView) {
        m_calculationSuggestion = {};
        return;
    }

    const QTextCursor cursor = editor->textCursor();
    m_calculationSuggestion = m_calculatorIntegration.evaluateExpression(cursor.block().text().trimmed());
    if (m_calculationSuggestion.valid) {
        calculatorSuggestionLabel->setText(QStringLiteral("Enter = %1").arg(m_calculationSuggestion.result));
        calculatorSuggestionLabel->show();
    } else {
        calculatorSuggestionLabel->clear();
        calculatorSuggestionLabel->hide();
    }
}

bool NotesEditorWidget::insertCalculationResult()
{
    if (m_lockedView || !editor)
        return false;

    const QTextCursor currentCursor = editor->textCursor();
    const CalculationSuggestion suggestion =
        m_calculatorIntegration.evaluateExpression(currentCursor.block().text().trimmed());
    if (!suggestion.valid)
        return false;

    m_markdownFormatting = true;
    QTextCursor cursor = currentCursor;
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.insertText(suggestion.replacement);
    cursor.endEditBlock();
    editor->setTextCursor(cursor);
    m_markdownFormatting = false;

    updateDocumentStats();
    updateCalculationSuggestion();
    emitContentChanged();
    return true;
}

void NotesEditorWidget::updateAttachmentSummary()
{
    if (m_currentNote.attachments.isEmpty()) {
        attachmentsLabel->hide();
        attachmentsLabel->clear();
        return;
    }

    QStringList names;
    for (const NoteAttachment &attachment : m_currentNote.attachments)
        names << attachment.displayName;
    attachmentsLabel->setText(QStringLiteral("Attachments: %1").arg(names.join(QStringLiteral(", "))));
    attachmentsLabel->show();
}

bool NotesEditorWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == editor) {
        if (event->type() == QEvent::DragEnter) {
            auto *dragEvent = static_cast<QDragEnterEvent *>(event);
            if (dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }

        if (event->type() == QEvent::Drop) {
            auto *dropEvent = static_cast<QDropEvent *>(event);
            QStringList paths;
            for (const QUrl &url : dropEvent->mimeData()->urls()) {
                if (url.isLocalFile())
                    paths.append(url.toLocalFile());
            }
            if (!paths.isEmpty()) {
                emit filesDropped(paths);
                dropEvent->acceptProposedAction();
                return true;
            }
        }

        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->matches(QKeySequence::Paste)) {
                const QImage image = QApplication::clipboard()->image();
                if (!image.isNull()) {
                    emit imagePasteRequested(image);
                    return true;
                }
            }
            if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
                && insertCalculationResult()) {
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonRelease) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (toggleChecklistAtPosition(mouseEvent->pos()))
                    return true;

                const QTextCursor cursor = editor->cursorForPosition(mouseEvent->pos());
                const QString title = NoteLinkService::linkTitleAt(editor->toPlainText(), cursor.position());
                if (!title.isEmpty()) {
                    emit noteLinkActivated(title);
                    return true;
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

bool NotesEditorWidget::toggleChecklistAtPosition(const QPoint &position)
{
    if (m_lockedView || !editor)
        return false;

    QTextCursor cursor = editor->cursorForPosition(position);
    const QTextBlock block = cursor.block();
    const QString text = block.text();
    QRegularExpression regex(QStringLiteral(R"(^(\s*(?:-\s*)?\[)([ xX])(\]))"));
    const QRegularExpressionMatch match = regex.match(text);
    if (!match.hasMatch())
        return false;

    const int positionInBlock = cursor.position() - block.position();
    if (positionInBlock > match.capturedEnd(3))
        return false;

    const bool completed = match.captured(2).compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0;
    QTextCursor editCursor(block);
    editCursor.setPosition(block.position() + match.capturedStart(2));
    editCursor.setPosition(block.position() + match.capturedEnd(2), QTextCursor::KeepAnchor);
    editCursor.insertText(completed ? QStringLiteral(" ") : QStringLiteral("x"));
    editor->setTextCursor(editCursor);
    updateChecklistFormatting();
    emitContentChanged();
    return true;
}

void NotesEditorWidget::applyMarkdownShortcut()
{
    if (m_markdownFormatting || !editor)
        return;

    QTextCursor cursor = editor->textCursor();
    const QTextBlock block = cursor.block();
    const QString text = block.text();
    QString prefix;
    QTextCharFormat charFormat;
    QTextBlockFormat blockFormat;
    bool shouldFormat = false;

    if (text.startsWith(QStringLiteral("### "))) {
        prefix = QStringLiteral("### ");
        charFormat.setFontPointSize(15);
        charFormat.setFontWeight(QFont::DemiBold);
        blockFormat.setHeadingLevel(3);
        shouldFormat = true;
    } else if (text.startsWith(QStringLiteral("## "))) {
        prefix = QStringLiteral("## ");
        charFormat.setFontPointSize(18);
        charFormat.setFontWeight(QFont::Bold);
        blockFormat.setHeadingLevel(2);
        shouldFormat = true;
    } else if (text.startsWith(QStringLiteral("# "))) {
        prefix = QStringLiteral("# ");
        charFormat.setFontPointSize(24);
        charFormat.setFontWeight(QFont::Bold);
        blockFormat.setHeadingLevel(1);
        shouldFormat = true;
    } else if (text.startsWith(QStringLiteral("> "))) {
        prefix = QStringLiteral("> ");
        charFormat.setFontItalic(true);
        blockFormat.setLeftMargin(24);
        shouldFormat = true;
    } else if (text.startsWith(QStringLiteral("```"))) {
        prefix = QStringLiteral("```");
        charFormat.setFontFamilies({QStringLiteral("Consolas")});
        blockFormat.setLeftMargin(12);
        shouldFormat = true;
    }

    if (text.trimmed() == QStringLiteral("---")) {
        m_markdownFormatting = true;
        QTextCursor editCursor(block);
        editCursor.select(QTextCursor::BlockUnderCursor);
        editCursor.insertHtml(QStringLiteral("<hr/>"));
        m_markdownFormatting = false;
        return;
    }

    if (!shouldFormat || prefix.isEmpty())
        return;

    m_markdownFormatting = true;
    QTextCursor editCursor(block);
    editCursor.setPosition(block.position());
    editCursor.setPosition(block.position() + prefix.size(), QTextCursor::KeepAnchor);
    editCursor.removeSelectedText();
    editCursor.mergeBlockFormat(blockFormat);
    editCursor.mergeCharFormat(charFormat);
    editor->mergeCurrentCharFormat(charFormat);
    m_markdownFormatting = false;
}

void NotesEditorWidget::updateChecklistFormatting()
{
    if (m_markdownFormatting || !editor)
        return;

    m_markdownFormatting = true;
    QTextBlock block = editor->document()->firstBlock();
    while (block.isValid()) {
        const QString text = block.text();
        const bool isChecklist = text.trimmed().startsWith(QStringLiteral("- ["))
                                 || text.trimmed().startsWith(QStringLiteral("["));
        if (isChecklist) {
            const bool completed = text.contains(QStringLiteral("[x]"), Qt::CaseInsensitive);
            QTextCursor cursor(block);
            cursor.select(QTextCursor::BlockUnderCursor);
            QTextCharFormat format;
            format.setFontStrikeOut(completed);
            cursor.mergeCharFormat(format);
        }
        block = block.next();
    }
    m_markdownFormatting = false;
}

void NotesEditorWidget::mergeFormat(const QTextCharFormat &format)
{
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    editor->mergeCurrentCharFormat(format);
    editor->setTextCursor(cursor);
    editor->setFocus();
}

void NotesEditorWidget::applyBlockStyle(const QString &styleName)
{
    if (m_loading || !editor)
        return;

    QTextCursor cursor = editor->textCursor();
    QTextCharFormat charFormat;
    QTextBlockFormat blockFormat;
    blockFormat.setHeadingLevel(0);

    if (styleName == QStringLiteral("title")) {
        charFormat.setFontPointSize(24);
        charFormat.setFontWeight(QFont::Bold);
        blockFormat.setHeadingLevel(1);
    } else if (styleName == QStringLiteral("heading")) {
        charFormat.setFontPointSize(18);
        charFormat.setFontWeight(QFont::Bold);
        blockFormat.setHeadingLevel(2);
    } else if (styleName == QStringLiteral("subheading")) {
        charFormat.setFontPointSize(15);
        charFormat.setFontWeight(QFont::DemiBold);
        blockFormat.setHeadingLevel(3);
    } else if (styleName == QStringLiteral("mono")) {
        charFormat.setFontFamilies({QStringLiteral("Consolas")});
        charFormat.setFontPointSize(12);
    } else {
        charFormat.setFontPointSize(12);
        charFormat.setFontWeight(QFont::Normal);
        charFormat.setFontFamilies({QStringLiteral("Segoe UI")});
    }

    cursor.mergeBlockFormat(blockFormat);
    cursor.mergeCharFormat(charFormat);
    editor->mergeCurrentCharFormat(charFormat);
    editor->setTextCursor(cursor);
    editor->setFocus();
}

void NotesEditorWidget::setEditorEnabledForLock(bool locked)
{
    titleEdit->setReadOnly(locked);
    editor->setReadOnly(locked);
    styleCombo->setEnabled(!locked);
    btnBold->setEnabled(!locked);
    btnItalic->setEnabled(!locked);
    btnUnderline->setEnabled(!locked);
    btnStrike->setEnabled(!locked);
    btnHighlight->setEnabled(!locked);
    btnBulletList->setEnabled(!locked);
    btnNumberedList->setEnabled(!locked);
    btnChecklist->setEnabled(!locked);
    btnTable->setEnabled(!locked);
    btnLink->setEnabled(!locked);
    btnAttach->setEnabled(!locked);
    btnLock->setText(locked ? QStringLiteral("Unlock") : QStringLiteral("Lock"));
}
