// noteseditorwidget.h
#ifndef NOTESEDITORWIDGET_H
#define NOTESEDITORWIDGET_H

#include "calculatorintegration.h"
#include "notesmanager.h"

#include <QImage>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QTextCharFormat>

class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QComboBox;

class NotesEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NotesEditorWidget(QWidget *parent = nullptr);

    QString title() const;
    QString plainText() const;
    QString html() const;
    QStringList detectedTags() const;
    void setEditorFontSize(int pointSize);
    void setAutosaveText(const QString &text);

public slots:
    void loadNote(const Note &note, bool locked);
    void loadNote(const QString &title, const QString &body);
    void clearEditor();
    void focusTitleAndSelectAll();
    void insertAttachmentReference(const NoteAttachment &attachment);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void contentChanged(const QString &title,
                        const QString &plainText,
                        const QString &html,
                        const QStringList &tags);
    void titleChanged(const QString &title);
    void bodyChanged(const QString &body);
    void attachRequested();
    void lockRequested();
    void deleteRequested();
    void exportRequested();
    void printRequested();
    void noteLinkActivated(const QString &title);
    void filesDropped(const QStringList &filePaths);
    void imagePasteRequested(const QImage &image);

private:
    void buildUI();
    void buildToolbar();
    void buildStatusBar();
    void connectSignals();
    void emitContentChanged();
    void updateDocumentStats();
    void updateAttachmentSummary();
    void updateCalculationSuggestion();
    bool insertCalculationResult();
    bool toggleChecklistAtPosition(const QPoint &position);
    void applyMarkdownShortcut();
    void updateChecklistFormatting();
    void mergeFormat(const QTextCharFormat &format);
    void applyBlockStyle(const QString &styleName);
    void setEditorEnabledForLock(bool locked);

    QLineEdit *titleEdit = nullptr;
    QWidget *editorToolbar = nullptr;
    QComboBox *styleCombo = nullptr;
    QTextEdit *editor = nullptr;
    QLabel *attachmentsLabel = nullptr;

    QPushButton *btnBold = nullptr;
    QPushButton *btnItalic = nullptr;
    QPushButton *btnUnderline = nullptr;
    QPushButton *btnStrike = nullptr;
    QPushButton *btnHighlight = nullptr;
    QPushButton *btnBulletList = nullptr;
    QPushButton *btnNumberedList = nullptr;
    QPushButton *btnChecklist = nullptr;
    QPushButton *btnTable = nullptr;
    QPushButton *btnLink = nullptr;
    QPushButton *btnAttach = nullptr;
    QPushButton *btnLock = nullptr;
    QPushButton *btnExport = nullptr;
    QPushButton *btnPrint = nullptr;
    QPushButton *btnDelete = nullptr;

    QWidget *statusBarPanel = nullptr;
    QLabel *wordCountLabel = nullptr;
    QLabel *characterCountLabel = nullptr;
    QLabel *readingTimeLabel = nullptr;
    QLabel *checklistProgressLabel = nullptr;
    QLabel *dateLabel = nullptr;
    QLabel *calculatorSuggestionLabel = nullptr;
    QLabel *tagCountLabel = nullptr;
    QLabel *autosavedLabel = nullptr;

    Note m_currentNote;
    CalculatorIntegration m_calculatorIntegration;
    CalculationSuggestion m_calculationSuggestion;
    bool m_loading = false;
    bool m_lockedView = false;
    bool m_markdownFormatting = false;
};

#endif // NOTESEDITORWIDGET_H
