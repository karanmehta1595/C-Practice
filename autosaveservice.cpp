// autosaveservice.cpp
#include "autosaveservice.h"

#include <QTimer>

AutoSaveService::AutoSaveService(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &AutoSaveService::saveRequested);
}

bool AutoSaveService::enabled() const
{
    return m_enabled;
}

void AutoSaveService::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
        stop();
}

int AutoSaveService::intervalMs() const
{
    return m_intervalMs;
}

void AutoSaveService::setIntervalMs(int intervalMs)
{
    m_intervalMs = qMax(250, intervalMs);
}

void AutoSaveService::schedule()
{
    if (m_enabled)
        m_timer->start(m_intervalMs);
}

void AutoSaveService::stop()
{
    m_timer->stop();
}
