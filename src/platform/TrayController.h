#pragma once
#include <QObject>
#include <QSystemTrayIcon>

class QAction;
class QMenu;
class AppSettings;
class Localizer;
class Scheduler;
struct ReleaseInfo;

/** System tray icon with quick actions: Next, Pause/Resume, Settings, Check for updates, Quit. */
class TrayController : public QObject
{
    Q_OBJECT
public:
    TrayController(Localizer &loc, Scheduler &scheduler, QObject *parent = nullptr);
    ~TrayController() override;

    void show();
    QSystemTrayIcon *icon() const { return m_tray; }

public slots:
    void showMessage(const QString &title, const QString &text);
    void showUpdateAvailable(const ReleaseInfo &info);

signals:
    void openSettingsRequested();
    void nextDhikrRequested();
    void pauseResumeRequested();
    void checkUpdatesRequested();

private slots:
    void retranslate();
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    Localizer &m_loc;
    Scheduler &m_scheduler;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_actNext = nullptr, *m_actPause = nullptr, *m_actSettings = nullptr,
            *m_actUpdates = nullptr, *m_actQuit = nullptr;
};
