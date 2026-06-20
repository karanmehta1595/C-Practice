#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonArray>

struct HistEntry { 
    QString text; 
    QDateTime when; 
};

class HistoryManager : public QObject {
    Q_OBJECT
public:
    explicit HistoryManager(QObject *parent = nullptr);
    static HistoryManager* instance();

    void loadHistory();
    void saveHistoryToDisk();
    void addToHistory(const QString &entry);
    void clearHistory();
    
    QList<HistEntry> getEntries() const;
    void setEntriesFromJson(const QJsonArray &arr);
    QJsonArray getEntriesAsJson() const;

    QString dateHeaderFor(const QDateTime &dt) const;
    bool histMatches(const HistEntry &e, const QString &qLower) const;

signals:
    void historyChanged();

private:
    static HistoryManager* s_instance;
    QList<HistEntry> m_historyEntries;
};

#endif // HISTORYMANAGER_H