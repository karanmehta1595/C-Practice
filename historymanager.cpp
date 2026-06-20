#include "historymanager.h"
#include <QSettings>
#include <QDate>
#include <QJsonObject>

HistoryManager* HistoryManager::s_instance = nullptr;

HistoryManager::HistoryManager(QObject *parent) : QObject(parent) {
    s_instance = this;
}

HistoryManager* HistoryManager::instance() {
    if (!s_instance) s_instance = new HistoryManager();
    return s_instance;
}

void HistoryManager::loadHistory() {
    QSettings s("1OS", "Calculator");
    int hCount = s.beginReadArray("history");
    m_historyEntries.clear();
    for (int i = 0; i < hCount && i < 500; i++) {
        s.setArrayIndex(i);
        HistEntry e;
        e.text = s.value("entry").toString();
        QString ts = s.value("ts").toString();
        e.when = ts.isEmpty() ? QDateTime() : QDateTime::fromString(ts, Qt::ISODate);
        m_historyEntries << e;
    }
    s.endArray();
    emit historyChanged();
}

void HistoryManager::saveHistoryToDisk() {
    QSettings s("1OS", "Calculator");
    s.beginWriteArray("history");
    for (int i = 0; i < m_historyEntries.size(); i++) {
        s.setArrayIndex(i);
        s.setValue("entry", m_historyEntries[i].text);
        s.setValue("ts", m_historyEntries[i].when.isValid() ? m_historyEntries[i].when.toString(Qt::ISODate) : QString());
    }
    s.endArray();
}

void HistoryManager::addToHistory(const QString &entry) {
    HistEntry e{ entry, QDateTime::currentDateTime() };
    m_historyEntries.prepend(e);
    if (m_historyEntries.size() > 500) m_historyEntries.removeLast();
    saveHistoryToDisk();
    emit historyChanged();
}

void HistoryManager::clearHistory() {
    m_historyEntries.clear();
    saveHistoryToDisk();
    emit historyChanged();
}

QList<HistEntry> HistoryManager::getEntries() const {
    return m_historyEntries;
}

void HistoryManager::setEntriesFromJson(const QJsonArray &arr) {
    m_historyEntries.clear();
    for (const auto &v : arr) {
        QJsonObject o = v.toObject();
        HistEntry e;
        e.text = o.value("entry").toString();
        QString ts = o.value("ts").toString();
        e.when = ts.isEmpty() ? QDateTime() : QDateTime::fromString(ts, Qt::ISODate);
        m_historyEntries << e;
    }
    saveHistoryToDisk();
    emit historyChanged();
}

QJsonArray HistoryManager::getEntriesAsJson() const {
    QJsonArray arr;
    for (const auto &e : m_historyEntries) {
        QJsonObject o;
        o["entry"] = e.text;
        o["ts"] = e.when.isValid() ? e.when.toString(Qt::ISODate) : QString();
        arr.append(o);
    }
    return arr;
}

QString HistoryManager::dateHeaderFor(const QDateTime &dt) const {
    if (!dt.isValid()) return "Earlier";
    QDate d = dt.date();
    QDate today = QDate::currentDate();
    if (d == today)            return "Today";
    if (d == today.addDays(-1))return "Yesterday";
    if (d > today.addDays(-7)) return d.toString("dddd");
    return d.toString("dddd, dd MMMM yyyy");
}

bool HistoryManager::histMatches(const HistEntry &e, const QString &qLower) const {
    if (qLower.isEmpty()) return true;
    if (e.text.toLower().contains(qLower)) return true;
    if (e.when.isValid()) {
        const QStringList forms = {
            e.when.toString("dd MMM yyyy").toLower(),
            e.when.toString("dd MMMM yyyy").toLower(),
            e.when.toString("dd/MM/yyyy").toLower(),
            e.when.toString("hh:mm AP").toLower(),
            e.when.toString("dddd").toLower(),
            dateHeaderFor(e.when).toLower()
        };
        for (const QString &f : forms)
            if (f.contains(qLower)) return true;
    }
    return false;
}