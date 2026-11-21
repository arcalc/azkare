#include "Scheduler.h"
#include "AppSettings.h"
#include "AdhkarDatabase.h"
#include "../overlay/OverlayManager.h"
#include "../notifications/NotificationDispatcher.h"

Scheduler::Scheduler(AppSettings          &settings,
                     AdhkarDatabase       &db,
                     OverlayManager       &overlay,
                     NotificationDispatcher &notifier,
                     QObject              *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_db(db)
    , m_overlay(overlay)
    , m_notifier(notifier)
{
    connect(&m_rotationTimer, &QTimer::timeout, this, &Scheduler::onRotationTick);
    connect(&m_notifTimer,    &QTimer::timeout, this, &Scheduler::onNotificationTick);
}

void Scheduler::start()
{
    m_paused = false;

    // Show first entry immediately
    m_overlay.showEntry(m_db.current());

    m_rotationTimer.start(m_settings.rotationIntervalSecs() * 1000);

    if (m_settings.notificationsEnabled())
        m_notifTimer.start(m_settings.notificationIntervalSecs() * 1000);
}

void Scheduler::stop()
{
    m_rotationTimer.stop();
    m_notifTimer.stop();
}

void Scheduler::forceNext()
{
    onRotationTick();
    // Reset timer so the next auto-advance is a full interval away
    if (!m_paused)
        m_rotationTimer.start(m_settings.rotationIntervalSecs() * 1000);
}

void Scheduler::togglePause()
{
    m_paused = !m_paused;
    if (m_paused) {
        m_rotationTimer.stop();
        m_notifTimer.stop();
    } else {
        m_rotationTimer.start(m_settings.rotationIntervalSecs() * 1000);
        if (m_settings.notificationsEnabled())
            m_notifTimer.start(m_settings.notificationIntervalSecs() * 1000);
    }
    emit pauseStateChanged(m_paused);
}

void Scheduler::applyNewSettings()
{
    if (!m_paused) {
        m_rotationTimer.setInterval(m_settings.rotationIntervalSecs() * 1000);
        m_notifTimer.setInterval(m_settings.notificationIntervalSecs() * 1000);

        if (m_settings.notificationsEnabled())
            m_notifTimer.start();
        else
            m_notifTimer.stop();
    }
}

void Scheduler::onRotationTick()
{
    const AdhkarEntry &entry = m_db.next();
    m_overlay.showEntry(entry);
    emit dhikrChanged();
}

void Scheduler::onNotificationTick()
{
    const AdhkarEntry &entry = m_db.current();
    m_notifier.send(entry);
}
