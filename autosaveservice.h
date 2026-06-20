// autosaveservice.h
#ifndef AUTOSAVESERVICE_H
#define AUTOSAVESERVICE_H

#include <QObject>

class QTimer;

class AutoSaveService : public QObject
{
    Q_OBJECT

public:
    explicit AutoSaveService(QObject *parent = nullptr);

    bool enabled() const;
    void setEnabled(bool enabled);
    int intervalMs() const;
    void setIntervalMs(int intervalMs);
    void schedule();
    void stop();

signals:
    void saveRequested();

private:
    QTimer *m_timer = nullptr;
    bool m_enabled = true;
    int m_intervalMs = 1500;
};

#endif // AUTOSAVESERVICE_H
