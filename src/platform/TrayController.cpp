#include "TrayController.h"

#include "../core/Localizer.h"
#include "../core/Scheduler.h"
#include "../core/Updater.h"
#include "../ui/AppIcon.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QDebug>

TrayController::TrayController(Localizer &loc, Scheduler &scheduler, QObject *parent)
    : QObject(parent), m_loc(loc), m_scheduler(scheduler)
{
    m_tray = new QSystemTrayIcon(appIcon(), this);
    m_menu = new QMenu;

    m_actNext = m_menu->addAction(QString(), this, &TrayController::nextDhikrRequested);
    m_actPause = m_menu->addAction(QString(), this, &TrayController::pauseResumeRequested);
    m_menu->addSeparator();
    m_actSettings = m_menu->addAction(QString(), this, &TrayController::openSettingsRequested);
    m_actUpdates = m_menu->addAction(QString(), this, &TrayController::checkUpdatesRequested);
    m_menu->addSeparator();
    m_actQuit = m_menu->addAction(QString(), qApp, &QCoreApplication::quit);

    m_tray->setContextMenu(m_menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &TrayController::onActivated);
    connect(&m_loc, &Localizer::languageChanged, this, &TrayController::retranslate);
    connect(&m_scheduler, &Scheduler::pauseStateChanged, this, &TrayController::retranslate);
    retranslate();
}

TrayController::~TrayController() { delete m_menu; }

void TrayController::show()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        qWarning() << "[Tray] no system tray on this desktop - use the CLI (azkare --help) or `azkare --settings`";
    m_tray->show();
}

void TrayController::retranslate()
{
    m_actNext->setText(m_loc.t(QStringLiteral("tray.next_dhikr")));
    m_actPause->setText(m_loc.t(m_scheduler.isPaused() ? QStringLiteral("tray.resume") : QStringLiteral("tray.pause")));
    m_actSettings->setText(m_loc.t(QStringLiteral("tray.open_settings")));
    m_actUpdates->setText(m_loc.t(QStringLiteral("tray.check_updates")));
    m_actQuit->setText(m_loc.t(QStringLiteral("tray.quit")));
    m_tray->setToolTip(m_loc.t(QStringLiteral("app.name")));
}

void TrayController::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        emit openSettingsRequested();
    else if (reason == QSystemTrayIcon::MiddleClick)
        emit nextDhikrRequested();
}

void TrayController::showMessage(const QString &title, const QString &text)
{
    m_tray->showMessage(title, text, QSystemTrayIcon::Information, 8000);
}

void TrayController::showUpdateAvailable(const ReleaseInfo &info)
{
    showMessage(m_loc.t(QStringLiteral("tray.update_available")).arg(info.tag),
                m_loc.t(QStringLiteral("settings.about_update_cmd")) + QStringLiteral(" ") + info.installCommand);
}
