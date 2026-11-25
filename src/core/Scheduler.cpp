#include "Scheduler.h"

#include "AdhkarDatabase.h"
#include "AppSettings.h"
#include "../notifications/NotificationDispatcher.h"
#include "../overlay/OverlayManager.h"

Scheduler::Scheduler(AppSettings &settings, AdhkarDatabase &db, OverlayManager &overlay,
                     NotificationDispatcher &notifier, QObject *parent)
    : QObject(parent), m_settings(settings), m_db(db), m_overlay(overlay), m_notifier(notifier)
{
    connect(&m_rotationTimer, &QTimer::timeout, this, &Scheduler::onRotationTick);
    connect(&m_notifTimer, &QTimer::timeout, this, &Scheduler::onNotificationTick);
}

void Scheduler::start()
{
    m_paused = false;
    if (m_settings.overlayEnabled())
        m_overlay.showEntry(m_db.current());
    restartTimers();
}

void Scheduler::restartTimers()
{
    m_rotationTimer.stop();
    m_notifTimer.stop();
    if (m_paused)
        return;
    if (m_settings.overlayEnabled())
        m_rotationTimer.start(m_settings.rotationIntervalSecs() * 1000);
    if (m_settings.notificationsEnabled())
        m_notifTimer.start(m_settings.notificationIntervalSecs() * 1000);
}

void Scheduler::forceNext()
{
    const AdhkarEntry &e = m_db.next();
    if (m_settings.overlayEnabled())
        m_overlay.showEntry(e);
    else
        m_notifier.send(e); // notifications-only: "next" delivers one right away
    restartTimers();
}

void Scheduler::togglePause()
{
    m_paused = !m_paused;
    restartTimers();
    emit pauseStateChanged(m_paused);
}

void Scheduler::applyNewSettings()
{
    if (m_settings.overlayEnabled())
        m_overlay.showEntry(m_db.current());
    restartTimers();
}

void Scheduler::onRotationTick()
{
    m_overlay.showEntry(m_db.next());
}

void Scheduler::onNotificationTick()
{
    // With the overlay running the notification repeats what is on screen;
    // in notifications-only mode it advances on its own.
    const AdhkarEntry &e = m_settings.overlayEnabled() ? m_db.current() : m_db.next();
    m_notifier.send(e);
}
