#pragma once

#include <QObject>
#include <QTimer>

class AppSettings;
class AdhkarDatabase;
class OverlayManager;
class NotificationDispatcher;

/**
 * Central timer loop.  Advances the Adhkar database on the rotation
 * interval and fires notification dispatch on its own cadence.
 */
class Scheduler : public QObject
{
    Q_OBJECT

public:
    explicit Scheduler(AppSettings          &settings,
                       AdhkarDatabase       &db,
                       OverlayManager       &overlay,
                       NotificationDispatcher &notifier,
                       QObject              *parent = nullptr);

    void start();
    void stop();

public slots:
    void forceNext();
    void togglePause();
    void applyNewSettings(); ///< Call after settings change

signals:
    void dhikrChanged();
    void pauseStateChanged(bool paused);

private slots:
    void onRotationTick();
    void onNotificationTick();

private:
    AppSettings            &m_settings;
    AdhkarDatabase         &m_db;
    OverlayManager         &m_overlay;
    NotificationDispatcher &m_notifier;

    QTimer m_rotationTimer;
    QTimer m_notifTimer;
    bool   m_paused = false;
};
