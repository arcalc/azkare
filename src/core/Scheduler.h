#pragma once
#include <QObject>
#include <QTimer>

class AppSettings;
class AdhkarDatabase;
class OverlayManager;
class NotificationDispatcher;

/** Drives overlay rotation and notification cadence according to the display mode. */
class Scheduler : public QObject
{
    Q_OBJECT
public:
    Scheduler(AppSettings &settings, AdhkarDatabase &db, OverlayManager &overlay,
              NotificationDispatcher &notifier, QObject *parent = nullptr);

    void start();
    bool isPaused() const { return m_paused; }

public slots:
    void forceNext();
    void togglePause();
    void applyNewSettings(); ///< re-read intervals / mode and restart timers

signals:
    void pauseStateChanged(bool paused);

private slots:
    void onRotationTick();
    void onNotificationTick();

private:
    void restartTimers();

    AppSettings &m_settings;
    AdhkarDatabase &m_db;
    OverlayManager &m_overlay;
    NotificationDispatcher &m_notifier;
    QTimer m_rotationTimer;
    QTimer m_notifTimer;
    bool m_paused = false;
};
