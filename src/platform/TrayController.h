#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class AppSettings;
class Scheduler;
class OverlayManager;
class NotificationDispatcher;
struct ReleaseInfo;

/**
 * Manages the system tray icon, its context menu, and quick-action signals.
 */
class TrayController : public QObject
{
    Q_OBJECT

public:
    explicit TrayController(AppSettings            &settings,
                            Scheduler              &scheduler,
                            OverlayManager         &overlay,
                            NotificationDispatcher &notifier,
                            QObject                *parent = nullptr);

    void show();
    void updatePauseLabel(bool paused);

public slots:
    void showUpdateNotification(const ReleaseInfo &info);

signals:
    void openSettingsRequested();
    void nextDhikrRequested();
    void pauseResumeRequested();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void buildMenu();
    void retranslate();

    AppSettings            &m_settings;
    Scheduler              &m_scheduler;
    OverlayManager         &m_overlay;
    NotificationDispatcher &m_notifier;

    QSystemTrayIcon *m_tray   = nullptr;
    QMenu           *m_menu   = nullptr;

    QAction *m_actNext        = nullptr;
    QAction *m_actPause       = nullptr;
    QAction *m_actSettings    = nullptr;
    QAction *m_actUpdate      = nullptr;
    QAction *m_actQuit        = nullptr;
};
